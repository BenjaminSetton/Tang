
#include "renderer.h"

// DISABLE WARNINGS FROM GLM AND STB_IMAGE DEPENDENCIES
#pragma warning( push )
#pragma warning( disable : 4201 4244)

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_ALIGNED_GENTYPES
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <gtx/hash.hpp>
#include <gtx/euler_angles.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#pragma warning( pop ) 

#include <array>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <glm.hpp>
#include <iostream>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

#include "asset_loader.h"
#include "data_buffer/vertex_buffer.h"
#include "data_buffer/index_buffer.h"
#include "descriptors/write_descriptor_set.h"

static constexpr uint32_t WINDOW_WIDTH = 1920;
static constexpr uint32_t WINDOW_HEIGHT = 1080;
static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
static constexpr uint32_t MAX_ASSET_COUNT = 100;

static std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

static std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
static const bool enableValidationLayers = false;
#else
static const bool enableValidationLayers = true;
#endif

static std::vector<char> readFile(const std::string& fileName)
{
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

static VkVertexInputBindingDescription GetVertexBindingDescription()
{
	VkVertexInputBindingDescription bindingDesc{};
	bindingDesc.binding = 0;
	bindingDesc.stride = sizeof(TANG::VertexType);
	bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDesc;
}

// Ensure that whenever we update the Vertex layout, we fail to compile unless
// the attribute descriptions below are updated. Note in this case we won't
// assert if the byte usage remains the same but we switch to a different format
// (like switching the order of two attributes)
static_assert(sizeof(TANG::VertexType) == 32);

static std::array<VkVertexInputAttributeDescription, 3> GetVertexAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 (12 bytes)
	attributeDescriptions[0].offset = offsetof(TANG::VertexType, pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 (12 bytes)
	attributeDescriptions[1].offset = offsetof(TANG::VertexType, normal);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT; // vec2 (8 bytes)
	attributeDescriptions[2].offset = offsetof(TANG::VertexType, uv);

	return attributeDescriptions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
)
{
	UNUSED(pUserData);
	UNUSED(messageType);
	UNUSED(messageSeverity);

	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}


VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}


namespace TANG
{
	struct QueueFamilyIndices
	{
		typedef uint32_t QueueFamilyIndexType;

		// If anything changes with the QueueType enum, make sure to change this struct as well!
		static_assert(static_cast<uint32_t>(QUEUE_COUNT) == 3);

		QueueFamilyIndices()
		{
			queueFamilies[GRAPHICS_QUEUE] = std::numeric_limits<QueueFamilyIndexType>::max();
			queueFamilies[PRESENT_QUEUE] = std::numeric_limits<QueueFamilyIndexType>::max();
			queueFamilies[TRANSFER_QUEUE] = std::numeric_limits<QueueFamilyIndexType>::max();
		}

		~QueueFamilyIndices()
		{
			// Nothing to do here
		}

		QueueFamilyIndices(const QueueFamilyIndices& other)
		{
			queueFamilies[GRAPHICS_QUEUE] = other.queueFamilies.at(GRAPHICS_QUEUE);
			queueFamilies[PRESENT_QUEUE] = other.queueFamilies.at(PRESENT_QUEUE);
			queueFamilies[TRANSFER_QUEUE] = other.queueFamilies.at(TRANSFER_QUEUE);
		}

		std::unordered_map<QueueType, QueueFamilyIndexType> queueFamilies;

		bool IsValid(QueueFamilyIndexType index)
		{
			return index != std::numeric_limits<QueueFamilyIndexType>::max();
		}

		bool IsComplete()
		{
			return	IsValid(queueFamilies[GRAPHICS_QUEUE]) &&
					IsValid(queueFamilies[PRESENT_QUEUE]) &&
					IsValid(queueFamilies[TRANSFER_QUEUE]);
		}
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	// This UBO is updated every frame for every different asset, to properly reflect their location. Matches
	// up with the Transform struct inside asset_types.h
	struct TransformUBO
	{
		TransformUBO() : transform(glm::identity<glm::mat4>())
		{
		}

		TransformUBO(glm::mat4 trans) : transform(trans)
		{
		}

		glm::mat4 transform;
	};

	struct ViewProjUBO
	{
		glm::mat4 view;
		glm::mat4 proj;
	};

	// The minimum uniform buffer alignment of the chosen physical device is 64 bytes...an entire matrix 4
	// 
	struct CameraDataUBO
	{
		glm::vec4 cameraPos;
		char padding[48];
	};
	TNG_ASSERT_COMPILE(sizeof(CameraDataUBO) == 64);

	void Renderer::Update(float* deltaTime)
	{
		UNUSED(deltaTime);

		/*static auto startTime = std::chrono::high_resolution_clock::now();

		float elapsedTime = std::chrono::duration<float, std::chrono::seconds::period>(std::chrono::high_resolution_clock::now() - startTime).count();
		float dt = deltaTime == nullptr ? elapsedTime : *deltaTime;*/

		glfwPollEvents();

	}

	void Renderer::Draw()
	{
		DrawFrame();

		// Clear the asset draw states after drawing the current frame
		// TODO - This is pretty slow to do per-frame, so I need to find a better way to
		//        clear the asset draw states. Maybe a sorted pool would work better but
		//        I want to avoid premature optimization so this'll do for now
		for (auto& resources : assetResources)
		{
			resources.shouldDraw = false;
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	// Loads an asset which implies grabbing the vertices and indices from the asset container
	// and creating vertex/index buffers to contain them. It also includes creating all other
	// API objects necessary for rendering. Receives a pointer to a loaded asset. This function
	// assumes the caller handled a null asset correctly
	AssetResources* Renderer::CreateAssetResources(AssetDisk* asset)
	{
		assetResources.emplace_back(AssetResources());
		resourcesMap.insert({ asset->uuid, static_cast<uint32_t>(assetResources.size() - 1) });

		AssetResources& resources = assetResources.back();

		uint32_t meshCount = static_cast<uint32_t>(asset->meshes.size());

		TNG_ASSERT_MSG(meshCount == 1, "Multiple meshes per asset is not currently supported!");

		// Resize the vertex buffer and offset vector to the number of meshes
		resources.vertexBuffers.resize(meshCount);
		resources.offsets.resize(meshCount);

		uint64_t totalIndexCount = 0;
		uint32_t vBufferOffset = 0;

		for (uint32_t i = 0; i < meshCount; i++)
		{
			Mesh& currMesh = asset->meshes[i];

			// Create the vertex buffer
			uint64_t numBytes = currMesh.vertices.size() * sizeof(VertexType);

			VertexBuffer& vb = resources.vertexBuffers[i];
			vb.Create(physicalDevice, logicalDevice, numBytes);

			VkCommandBuffer commandBuffer = BeginSingleTimeCommands(commandPools[TRANSFER_QUEUE]);
			vb.CopyData(logicalDevice, commandBuffer, currMesh.vertices.data(), numBytes);
			EndSingleTimeCommands(commandBuffer, TRANSFER_QUEUE);

			// Create the index buffer
			numBytes = currMesh.indices.size() * sizeof(IndexType);

			IndexBuffer& ib = resources.indexBuffer;
			ib.Create(physicalDevice, logicalDevice, numBytes);

			commandBuffer = BeginSingleTimeCommands(commandPools[TRANSFER_QUEUE]);
			ib.CopyData(logicalDevice, commandBuffer, currMesh.indices.data(), numBytes);
			EndSingleTimeCommands(commandBuffer, TRANSFER_QUEUE);

			// Destroy the staging buffers
			vb.DestroyIntermediateBuffers(logicalDevice);
			ib.DestroyIntermediateBuffers(logicalDevice);

			// Accumulate the index count of this mesh;
			totalIndexCount += currMesh.indices.size();

			// Set the current offset and then increment
			resources.offsets[i] = vBufferOffset++;
		}

		// Insert the asset's uuid into the assetDrawState map. We do not render it
		// upon insertion by default
		resources.shouldDraw = false;
		resources.transform = Transform();
		resources.indexCount = totalIndexCount;
		resources.uuid = asset->uuid;

		// Create uniform buffers for this asset
		CreateUniformBuffers(resources.uuid);
		UpdateInfrequentUniformBuffers(resources.uuid);

		CreateDescriptorSets(resources.uuid);
		UpdateInfrequentDescriptorSets(resources.uuid);

		return &resources;
	}

	void Renderer::CreateAssetCommandBuffer(AssetResources* resources)
	{
		UUID assetID = resources->uuid;

		// For every swap-chain image, insert object into map and then grab a reference to it
		for (uint32_t i = 0; i < GetSWIDDSize(); i++)
		{
			auto& secondaryCmdBufferMap = GetSWIDDAtIndex(i)->secondaryCommandBuffer;
			// Ensure that there's not already an entry in the secondaryCommandBuffers map. We bail in case of a collision
			if (secondaryCmdBufferMap.find(assetID) != secondaryCmdBufferMap.end())
			{
				LogError("Attempted to create a secondary command buffer for an asset, but a secondary command buffer was already found for asset uuid %ull", assetID);
				return;
			}

			secondaryCmdBufferMap.emplace(assetID, SecondaryCommandBuffer());
			SecondaryCommandBuffer& commandBuffer = secondaryCmdBufferMap[assetID];
			commandBuffer.Create(logicalDevice, commandPools[GRAPHICS_QUEUE]);
		}
	}

	void Renderer::DestroyAssetBuffersHelper(AssetResources& resources)
	{
		// Destroy all vertex buffers
		for (auto& vb : resources.vertexBuffers)
		{
			vb.Destroy(logicalDevice);
		}

		// Destroy the index buffer
		resources.indexBuffer.Destroy(logicalDevice);
	}

	PrimaryCommandBuffer* Renderer::GetCurrentPrimaryBuffer()
	{
		return &GetCurrentFDD()->primaryCommandBuffer;
	}

	SecondaryCommandBuffer* Renderer::GetSecondaryCommandBufferAtIndex(uint32_t frameBufferIndex, UUID uuid)
	{
		return &GetSWIDDAtIndex(frameBufferIndex)->secondaryCommandBuffer.at(uuid);
	}

	VkFramebuffer Renderer::GetFramebufferAtIndex(uint32_t frameBufferIndex)
	{
		return GetSWIDDAtIndex(frameBufferIndex)->swapChainFramebuffer;
	}

	void Renderer::DestroyAssetResources(UUID uuid)
	{
		TNG_ASSERT_MSG(resourcesMap.find(uuid) != resourcesMap.end(), "Failed to find asset resources!");

		// Destroy the resources
		DestroyAssetBuffersHelper(assetResources[resourcesMap[uuid]]);

		// Remove resources from the vector
		uint32_t resourceIndex = static_cast<uint32_t>((&assetResources[resourcesMap[uuid]] - &assetResources[0]) / sizeof(assetResources));
		assetResources.erase(assetResources.begin() + resourceIndex);

		// Destroy reference to resources
		resourcesMap.erase(uuid);
	}

	void Renderer::DestroyAllAssetResources()
	{
		uint32_t numAssetResources = static_cast<uint32_t>(assetResources.size());

		for (uint32_t i = 0; i < numAssetResources; i++)
		{
			DestroyAssetBuffersHelper(assetResources[i]);
		}

		assetResources.clear();
		resourcesMap.clear();
	}

	bool Renderer::WindowShouldClose()
	{
		return glfwWindowShouldClose(windowHandle);
	}

	void Renderer::Initialize()
	{
		InitWindow();
		InitVulkan();
	}

	void Renderer::Shutdown()
	{
		ShutdownVulkan();
		ShutdownWindow();

	}

	void Renderer::SetAssetDrawState(UUID uuid)
	{
		if (resourcesMap.find(uuid) == resourcesMap.end())
		{
			// Undefined behavior. Maybe the asset resources were deleted but we somehow forgot to remove it from the assetDrawStates map?
			TNG_ASSERT_MSG(false, "Attempted to set asset draw state, but draw state doesn't exist in the map!");
		}

		assetResources[resourcesMap[uuid]].shouldDraw = true;
	}

	void Renderer::SetAssetTransform(UUID uuid, Transform& transform)
	{
		assetResources[resourcesMap[uuid]].transform = transform;
	}

	void Renderer::SetAssetPosition(UUID uuid, glm::vec3& position)
	{
		Transform& transform = assetResources[resourcesMap[uuid]].transform;
		transform.position = position;
	}

	void Renderer::SetAssetRotation(UUID uuid, glm::vec3& rotation)
	{
		Transform& transform = assetResources[resourcesMap[uuid]].transform;
		transform.rotation = rotation;
	}

	void Renderer::SetAssetScale(UUID uuid, glm::vec3& scale)
	{
		Transform& transform = assetResources[resourcesMap[uuid]].transform;
		transform.scale = scale;
	}

	void Renderer::InitWindow()
	{
		// Initialize the window
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		windowHandle = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "TANG", nullptr, nullptr);
		glfwSetWindowUserPointer(windowHandle, this);
		glfwSetFramebufferSizeCallback(windowHandle, framebufferResizeCallback);
	}

	void Renderer::InitVulkan()
	{
		frameDependentData.resize(MAX_FRAMES_IN_FLIGHT);

		// Initialize Vulkan-related objects
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateImageViews();
		CreateDescriptorSetLayouts();
		CreateDescriptorPool();
		CreateRenderPass();
		CreateGraphicsPipeline();
		CreateCommandPools();
		CreateTextureImage();
		CreateTextureImageView();
		CreateTextureSampler();
		CreateColorResources();
		CreateDepthResources();
		CreateFramebuffers();
		CreatePrimaryCommandBuffers(GRAPHICS_QUEUE);
		CreateSyncObjects();

		// Create the CameraDataUBO
		VkDeviceSize cameraUBOSize = sizeof(CameraDataUBO);
		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			UniformBuffer& cameraUBO = GetFDDAtIndex(i)->cameraDataUBO;
			cameraUBO.Create(physicalDevice, logicalDevice, cameraUBOSize);
			cameraUBO.MapMemory(logicalDevice, cameraUBOSize);
		}
	}

	void Renderer::ShutdownWindow()
	{
		glfwDestroyWindow(windowHandle);
		glfwTerminate();
	}

	void Renderer::ShutdownVulkan()
	{
		vkDeviceWaitIdle(logicalDevice);

		DestroyAllAssetResources();

		CleanupSwapChain();

		vkDestroySampler(logicalDevice, textureSampler, nullptr);
		vkDestroyImageView(logicalDevice, textureImageView, nullptr);

		vkDestroyImage(logicalDevice, textureImage, nullptr);
		vkFreeMemory(logicalDevice, textureImageMemory, nullptr);

		for (auto& setLayout : setLayouts)
		{
			setLayout.Destroy(logicalDevice);
		}
		setLayouts.clear();

		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			auto frameData = GetFDDAtIndex(i);
			for (auto& iter : frameData->assetDescriptorDataMap)
			{
				iter.second.viewProjUBO.Destroy(logicalDevice);
				iter.second.transformUBO.Destroy(logicalDevice);
			}
			frameData->cameraDataUBO.Destroy(logicalDevice);
		}

		descriptorPool.Destroy(logicalDevice);

		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			auto frameData = GetFDDAtIndex(i);
			vkDestroySemaphore(logicalDevice, frameData->imageAvailableSemaphore, nullptr);
			vkDestroySemaphore(logicalDevice, frameData->renderFinishedSemaphore, nullptr);
			vkDestroyFence(logicalDevice, frameData->inFlightFence, nullptr);
		}

		// Destroy all command pools
		for (const auto& iter : commandPools)
		{
			vkDestroyCommandPool(logicalDevice, iter.second, nullptr);
		}
		commandPools.clear();

		vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
		vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

		vkDestroyDevice(logicalDevice, nullptr);
		vkDestroySurfaceKHR(vkInstance, surface, nullptr);

		if (enableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, nullptr);
		}

		vkDestroyInstance(vkInstance, nullptr);
	}

	void Renderer::DrawFrame()
	{
		VkResult result = VK_SUCCESS;

		FrameDependentData* currentFDD = GetCurrentFDD();

		vkWaitForFences(logicalDevice, 1, &currentFDD->inFlightFence, VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		result = vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX,
			currentFDD->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			TNG_ASSERT_MSG(false, "Failed to acquire swap chain image!");
		}

		// Only reset the fence if we're submitting work, otherwise we might deadlock
		vkResetFences(logicalDevice, 1, &(currentFDD->inFlightFence));

		///////////////////////////////////////
		// 
		// Record and submit primary command buffer
		//
		///////////////////////////////////////
		RecordPrimaryCommandBuffer(imageIndex);

		VkCommandBuffer commandBuffers[] = { GetCurrentPrimaryBuffer()->GetBuffer() };
		VkSemaphore waitSemaphores[] = { currentFDD->imageAvailableSemaphore };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = commandBuffers;

		VkSemaphore signalSemaphores[] = { currentFDD->renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(queues[GRAPHICS_QUEUE], 1, &submitInfo, currentFDD->inFlightFence) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to submit draw command buffer!");
			return;
		}

		///////////////////////////////////////
		// 
		// Swap chain present
		//
		///////////////////////////////////////
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		result = vkQueuePresentKHR(queues[PRESENT_QUEUE], &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
		{
			framebufferResized = false;
			RecreateSwapChain();
		}
		else if (result != VK_SUCCESS)
		{
			LogError("Failed to present swap chain image!");
		}
	}

	void Renderer::CreateInstance()
	{
		// Check that we support all requested validation layers
		if (enableValidationLayers && !CheckValidationLayerSupport())
		{
			throw std::runtime_error("Validation layers were requested, but one or more is not supported!");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "TANG";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		auto extensions = GetRequiredExtensions();

		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();
		createInfo.enabledLayerCount = 0;

		if (vkCreateInstance(&createInfo, nullptr, &vkInstance) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create Vulkan instance!");
		}
	}

	void Renderer::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr; // Optional
	}

	void Renderer::SetupDebugMessenger()
	{
		if (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		PopulateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(vkInstance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to setup debug messenger!");
		}
	}

	std::vector<const char*> Renderer::GetRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	bool Renderer::CheckValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				return false;
			}
		}

		return true;
	}

	bool Renderer::CheckDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	////////////////////////////////////////
	//
	//  PHYSICAL DEVICE
	//
	////////////////////////////////////////
	void Renderer::PickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);

		if (deviceCount == 0)
		{
			TNG_ASSERT_MSG(false, "Failed to find GPU with Vulkan support");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());

		for (const auto& device : devices)
		{
			if (IsDeviceSuitable(device))
			{
				physicalDevice = device;
				msaaSamples = GetMaxUsableSampleCount();
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE)
		{
			TNG_ASSERT_MSG(false, "Failed to find suitable device (GPU)!");
		}
	}

	bool Renderer::IsDeviceSuitable(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices = FindQueueFamilies(device);
		bool extensionsSupported = CheckDeviceExtensionSupport(device);
		bool swapChainAdequate = false;
		if (extensionsSupported)
		{
			SwapChainSupportDetails details = QuerySwapChainSupport(device);
			swapChainAdequate = !details.formats.empty() && !details.presentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		return indices.IsComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;

		//
		// THE CODE BELOW IS AN EXAMPLE OF HOW TO SELECT DEDICATED GPUS ONLY, WHILE
		// IGNORING INTEGRATED GPUS. FOR THE SAKE OF THIS EXAMPLE, WE'LL CONSIDER ALL
		// GPUS TO BE SUITABLE.
		//

		//VkPhysicalDeviceProperties properties;
		//vkGetPhysicalDeviceProperties(device, &properties);

		//VkPhysicalDeviceFeatures features;
		//vkGetPhysicalDeviceFeatures(device, &features);

		//// We're only going to say that dedicated GPUs are suitable, let's not deal with
		//// integrated graphics for now
		//return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
	}

	QueueFamilyIndices Renderer::FindQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		uint32_t graphicsTransferQueue = std::numeric_limits<uint32_t>::max();
		uint32_t i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			// Check that the device supports a graphics queue
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.queueFamilies[GRAPHICS_QUEUE] = i;
			}

			// Check that the device supports present queues
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport)
			{
				indices.queueFamilies[PRESENT_QUEUE] = i;
			}

			// Check that the device supports a transfer queue
			if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				// Choose a different queue from the graphics queue, if possible
				if (indices.queueFamilies[GRAPHICS_QUEUE] == i)
				{
					graphicsTransferQueue = i;
				}
				else
				{
					indices.queueFamilies[TRANSFER_QUEUE] = i;
				}
			}

			if (indices.IsComplete()) break;

			i++;
		}

		// If we couldn't find a different queue for the TRANSFER and GRAPHICS operations, then simply
		// use the same queue for both (if it supports TRANSFER operations)
		if (!indices.IsValid(TRANSFER_QUEUE) && graphicsTransferQueue != std::numeric_limits<uint32_t>::max())
		{
			indices.queueFamilies[TRANSFER_QUEUE] = graphicsTransferQueue;
		}

		// Check that we filled in all of our queue families, otherwise log a warning
		// NOTE - I'm not sure if there's a problem with not finding queue families for
		//        every queue type...
		if (!indices.IsComplete())
		{
			LogWarning("Failed to find all queue families!");
		}

		return indices;
	}

	SwapChainSupportDetails Renderer::QuerySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR Renderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		// We don't check if available formats is empty!
		return availableFormats[0];
	}

	VkPresentModeKHR Renderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& presentMode : availablePresentModes)
		{
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return presentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D Renderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		else
		{
			int width, height;
			glfwGetFramebufferSize(windowHandle, &width, &height);

			VkExtent2D actualExtent =
			{
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
				capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
				capabilities.maxImageExtent.height);

			return actualExtent;

		}
	}

	uint32_t Renderer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		TNG_ASSERT_MSG(false, "Failed to find suitable memory type!");
		return std::numeric_limits<uint32_t>::max();
	}

	////////////////////////////////////////
	//
	//  LOGICAL DEVICE
	//
	////////////////////////////////////////
	void Renderer::CreateLogicalDevice()
	{
		QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
		if (!indices.IsComplete())
		{
			LogError("Failed to create logical device because the queue family indices are incomplete!");
		}

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = {
			indices.queueFamilies[GRAPHICS_QUEUE],
			indices.queueFamilies[PRESENT_QUEUE],
			indices.queueFamilies[TRANSFER_QUEUE]
		};

		// TODO - Determine priority of the different queue types
		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create the logical device!");
		}

		// Get the queues from the logical device
		vkGetDeviceQueue(logicalDevice, indices.queueFamilies[GRAPHICS_QUEUE], 0, &queues[GRAPHICS_QUEUE]);
		vkGetDeviceQueue(logicalDevice, indices.queueFamilies[PRESENT_QUEUE], 0, &queues[PRESENT_QUEUE]);
		vkGetDeviceQueue(logicalDevice, indices.queueFamilies[TRANSFER_QUEUE], 0, &queues[TRANSFER_QUEUE]);

	}

	////////////////////////////////////////
	//
	//  SURFACE
	//
	////////////////////////////////////////
	void Renderer::CreateSurface()
	{
		if (glfwCreateWindowSurface(vkInstance, windowHandle, nullptr, &surface) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create window surface!");
		}
	}

	void Renderer::CreateSwapChain()
	{
		SwapChainSupportDetails details = QuerySwapChainSupport(physicalDevice);
		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(details.formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(details.presentModes);
		VkExtent2D extent = ChooseSwapExtent(details.capabilities);

		uint32_t imageCount = details.capabilities.minImageCount + 1;
		if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount)
		{
			imageCount = details.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[2] = { indices.queueFamilies[GRAPHICS_QUEUE], indices.queueFamilies[PRESENT_QUEUE]};

		if (queueFamilyIndices[0] != queueFamilyIndices[1])
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		createInfo.preTransform = details.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);

		swapChainImageDependentData.resize(imageCount);
		std::vector<VkImage> swapChainImages(imageCount);

		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());

		for (uint32_t i = 0; i < GetSWIDDSize(); i++)
		{
			swapChainImageDependentData[i].swapChainImage = swapChainImages[i];
		}
		swapChainImages.clear();

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	// Create image views for all images on the swap chain
	void Renderer::CreateImageViews()
	{
		auto& swidd = swapChainImageDependentData;
		for (size_t i = 0; i < GetSWIDDSize(); i++)
		{
			swidd[i].swapChainImageView = CreateImageView(swidd[i].swapChainImage, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}
	}

	// This is a helper function for creating the "VkShaderModule" wrappers around
	// the shader code, read from CreateGraphicsPipeline() below
	VkShaderModule Renderer::CreateShaderModule(std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create shader module!");
		}

		return shaderModule;
	}

	void Renderer::CreateGraphicsPipeline()
	{
		// Read the compiled shaders
		auto vertShaderCode = readFile("../out/shaders/vert.spv");
		auto fragShaderCode = readFile("../out/shaders/frag.spv");

		VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		// Vertex input
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};

		auto bindingDescription = GetVertexBindingDescription();
		auto attributeDescriptions = GetVertexAttributeDescriptions();
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		// Input assembler
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// Viewports and scissors
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		// We're declaring these as dynamic states, meaning we can change
		// them at any point. Usually the pipeline states in Vulkan are static,
		// meaning a pipeline is created and never changed. This allows
		// the GPU to heavily optimize for the pipelines defined. In this
		// case though, we face a negligible penalty for making these dynamic.
		std::vector<VkDynamicState> dynamicStates =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		// Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		// For the polygonMode it's possible to use LINE or POINT as well
		// In this case the following line is required:
		rasterizer.lineWidth = 1.0f;
		// Any line thicker than 1.0 requires the "wideLines" GPU feature
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		// Multisampling
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = msaaSamples;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		// Color blending
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		// Depth stencil
		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f; // Optional
		depthStencil.maxDepthBounds = 1.0f; // Optional
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {}; // Optional
		depthStencil.back = {}; // Optional

		// Pipeline layout
		std::vector<VkDescriptorSetLayout> vkDescSetLayouts;
		for (auto& setLayout : setLayouts)
		{
			vkDescSetLayouts.push_back(setLayout.GetLayout());
		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(vkDescSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = vkDescSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create pipeline layout!");
		}

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create graphics pipeline!");
		}

		vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
		vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);

	}

	void Renderer::CreateRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = msaaSamples;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = FindDepthFormat();
		depthAttachment.samples = msaaSamples;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachmentResolve{};
		colorAttachmentResolve.format = swapChainImageFormat;
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentResolveRef{};
		colorAttachmentResolveRef.attachment = 2;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
		subpass.pResolveAttachments = &colorAttachmentResolveRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create render pass!");
		}


	}

	void Renderer::CreateFramebuffers()
	{
		auto& swidd = swapChainImageDependentData;

		for (size_t i = 0; i < GetSWIDDSize(); i++)
		{
			std::array<VkImageView, 3> attachments =
			{
				colorImageView,
				depthImageView,
				swidd[i].swapChainImageView
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &(swidd[i].swapChainFramebuffer)) != VK_SUCCESS)
			{
				TNG_ASSERT_MSG(false, "Failed to create framebuffer!");
			}
		}
	}

	void Renderer::CreateCommandPools()
	{
		QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);

		// Allocate the graphics command pool
		if (queueFamilyIndices.IsValid(GRAPHICS_QUEUE))
		{
			// Allocate the pool object in the map
			commandPools[GRAPHICS_QUEUE] = VkCommandPool();

			VkCommandPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			poolInfo.queueFamilyIndex = queueFamilyIndices.queueFamilies[GRAPHICS_QUEUE];

			if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPools[GRAPHICS_QUEUE]) != VK_SUCCESS)
			{
				LogError("Failed to create a graphics command pool!");
			}
		}
		else
		{
			TNG_ASSERT_MSG(false, "Failed to find a queue family supporting a graphics queue!");
		}

		// Allocate the transfer command pool
		if (queueFamilyIndices.IsValid(TRANSFER_QUEUE))
		{
			commandPools[TRANSFER_QUEUE] = VkCommandPool();

			VkCommandPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			// We support short-lived operations (transient bit) and we want to reset the command buffers after submissions
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			poolInfo.queueFamilyIndex = queueFamilyIndices.queueFamilies[TRANSFER_QUEUE];

			if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPools[TRANSFER_QUEUE]) != VK_SUCCESS)
			{
				TNG_ASSERT_MSG(false, "Failed to create a transfer command pool!");
			}
		}
		else
		{
			TNG_ASSERT_MSG(false, "Failed to find a queue family supporting a transfer queue!");
		}
	}

	void Renderer::CreatePrimaryCommandBuffers(QueueType poolType)
	{
		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			GetFDDAtIndex(i)->primaryCommandBuffer.Create(logicalDevice, commandPools[poolType]);
		}
	}

	void Renderer::CreateSyncObjects()
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		// Creates the fence on the signaled state so we don't block on this fence for
		// the first frame (when we don't have any previous frames to wait on)
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &(GetFDDAtIndex(i)->imageAvailableSemaphore)) != VK_SUCCESS
				|| vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &(GetFDDAtIndex(i)->renderFinishedSemaphore)) != VK_SUCCESS
				|| vkCreateFence(logicalDevice, &fenceInfo, nullptr, &(GetFDDAtIndex(i)->inFlightFence)) != VK_SUCCESS)
			{
				TNG_ASSERT_MSG(false, "Failed to create semaphores or fences!");
			}
		}
	}

	void Renderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(logicalDevice, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to allocate memory for the buffer!");
		}

		vkBindBufferMemory(logicalDevice, buffer, bufferMemory, 0);
	}

	void Renderer::CreateUniformBuffers(UUID uuid)
	{
		VkDeviceSize transformUBOSize = sizeof(TransformUBO);
		VkDeviceSize viewProjUBOSize = sizeof(ViewProjUBO);

		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			FrameDependentData* currentFDD = GetFDDAtIndex(i);
			AssetDescriptorData& assetDescriptorData = currentFDD->assetDescriptorDataMap[uuid];

			UniformBuffer& transUBO = assetDescriptorData.transformUBO;

			// Create the TransformUBO
			transUBO.Create(physicalDevice, logicalDevice, transformUBOSize);
			transUBO.MapMemory(logicalDevice, transformUBOSize);

			// Create the ViewProjUBO
			UniformBuffer& vpUBO = assetDescriptorData.viewProjUBO;

			vpUBO.Create(physicalDevice, logicalDevice, viewProjUBOSize);
			vpUBO.MapMemory(logicalDevice, viewProjUBOSize);
		}
	}

	void Renderer::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format,
		VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
		VkImage& image, VkDeviceMemory& imageMemory)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = numSamples;

		if (vkCreateImage(logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(logicalDevice, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to allocate image memory!");
		}

		vkBindImageMemory(logicalDevice, image, imageMemory, 0);
	}

	void Renderer::CreateTextureImage()
	{
		int width, height, channels;
		stbi_uc* pixels = stbi_load("../src/data/textures/sample/texture.jpg", &width, &height, &channels, STBI_rgb_alpha);
		if (pixels == nullptr)
		{
			TNG_ASSERT_MSG(false, "Failed to load texture!");
		}

		textureMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

		VkDeviceSize imageSize = width * height * 4;
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(logicalDevice, stagingBufferMemory);

		// Now that we've copied over the texture data to the staging buffer, we don't need the original pixels array anymore
		stbi_image_free(pixels);

		CreateImage(width, height, textureMipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			textureImage, textureImageMemory);

		TransitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, textureMipLevels);
		CopyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
		GenerateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, width, height, textureMipLevels);

		vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
	}

	void Renderer::CreateDescriptorSetLayouts()
	{
		const uint32_t numSetLayouts = 2;
		setLayouts.resize(numSetLayouts);

		// Holds ViewProjUBO + ImageSampler
		setLayouts[0].AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
		setLayouts[0].AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		setLayouts[0].Create(logicalDevice);

		// Holds TransformUBO + CameraDataUBO
		setLayouts[1].AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
		setLayouts[1].AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
		setLayouts[1].Create(logicalDevice);
	}

	void Renderer::CreateDescriptorPool()
	{
		// We will create a descriptor pool that can allocate a large number of descriptor sets using the following logic:
		// Since we have to allocate a descriptor set for every unique asset (not sure if this is the correct way, to be honest)
		// and for every frame in flight, we'll set a maximum number of assets (100) and multiply that by the max number of frames
		// in flight
		// TODO - Once I learn how to properly set a different transform for every asset, this will probably have to change...I just
		//        don't know what I'm doing.
		const uint32_t fddSize = GetFDDSize();

		const uint32_t numUniformBuffers = 3;
		const uint32_t numImageSamplers = 1;

		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = numUniformBuffers * GetFDDSize();
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = numImageSamplers * GetFDDSize();

		descriptorPool.Create(logicalDevice, poolSizes.data(), static_cast<uint32_t>(poolSizes.size()), static_cast<uint32_t>(poolSizes.size()) * fddSize * MAX_ASSET_COUNT);
	}

	void Renderer::CreateDescriptorSets(UUID uuid)
	{
		uint32_t fddSize = GetFDDSize();

		for (uint32_t i = 0; i < fddSize; i++)
		{
			FrameDependentData* currentFDD = GetFDDAtIndex(i);
			currentFDD->assetDescriptorDataMap.insert({uuid, AssetDescriptorData()});
			AssetDescriptorData& assetDescriptorData = currentFDD->assetDescriptorDataMap[uuid];

			for (uint32_t j = 0; j < setLayouts.size(); j++)
			{
				assetDescriptorData.descriptorSets.push_back(DescriptorSet());
				assetDescriptorData.descriptorSets[j].Create(logicalDevice, descriptorPool, setLayouts[j]);
			}
		}

	}

	VkImageView Renderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = image;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = format;
		createInfo.subresourceRange.aspectMask = aspectFlags;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = mipLevels;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &imageView) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create texture image view!");
		}

		return imageView;
	}

	void Renderer::CreateTextureImageView()
	{
		textureImageView = CreateImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, textureMipLevels);
	}

	void Renderer::CreateTextureSampler()
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = static_cast<float>(textureMipLevels);

		if (vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create texture sampler!");
		}
	}

	void Renderer::CreateDepthResources()
	{
		VkFormat depthFormat = FindDepthFormat();
		CreateImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			depthImage, depthImageMemory);
		depthImageView = CreateImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
		//transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, mipLevels);
	}

	void Renderer::CreateColorResources()
	{
		VkFormat colorFormat = swapChainImageFormat;

		CreateImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, colorFormat,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
		colorImageView = CreateImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}

	void Renderer::RecordPrimaryCommandBuffer(uint32_t frameBufferIndex)
	{
		PrimaryCommandBuffer* commandBuffer = GetCurrentPrimaryBuffer();

		// Reset the primary buffer since we've marked it as one-time submit
		commandBuffer->Reset();

		// Primary command buffers don't need to define inheritance info
		commandBuffer->BeginRecording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);

		commandBuffer->CMD_BeginRenderPass(renderPass, GetFramebufferAtIndex(frameBufferIndex), swapChainExtent, true);

		// Execute the secondary commands here
		std::vector<VkCommandBuffer> secondaryCmdBuffers;
		secondaryCmdBuffers.resize(assetResources.size()); // At most we can have the same number of cmd buffers as there are asset resources
		uint32_t secondaryCmdBufferCount = 0;
		for (auto& iter : assetResources)
		{
			UUID& uuid = iter.uuid;

			if (assetResources[resourcesMap[uuid]].shouldDraw)
			{
				SecondaryCommandBuffer* secondaryCmdBuffer = GetSecondaryCommandBufferAtIndex(frameBufferIndex, uuid);

				UpdatePerFrameUniformBuffers(assetResources[resourcesMap[uuid]].transform, uuid);
				UpdatePerFrameDescriptorSets(uuid);

				secondaryCmdBuffer->Reset();
				RecordSecondaryCommandBuffer(*secondaryCmdBuffer, &iter, frameBufferIndex);

				secondaryCmdBuffers[secondaryCmdBufferCount++] = secondaryCmdBuffer->GetBuffer();
			}
		}

		// Don't attempt to execute 0 command buffers
		if (secondaryCmdBufferCount > 0)
		{
			commandBuffer->CMD_ExecuteSecondaryCommands(secondaryCmdBuffers.data(), secondaryCmdBufferCount);
		}

		commandBuffer->CMD_EndRenderPass();

		commandBuffer->EndRecording();
	}

	void Renderer::RecordSecondaryCommandBuffer(SecondaryCommandBuffer& commandBuffer, AssetResources* resources, uint32_t frameBufferIndex)
	{
		// Retrieve the vector of descriptor sets for the given asset
		auto& descSets = GetCurrentFDD()->assetDescriptorDataMap[resources->uuid].descriptorSets;
		std::vector<VkDescriptorSet> vkDescSets(descSets.size());
		for (uint32_t i = 0; i < descSets.size(); i++)
		{
			vkDescSets[i] = descSets[i].GetDescriptorSet();
		}

		VkCommandBufferInheritanceInfo inheritanceInfo{};
		inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.pNext = nullptr;
		inheritanceInfo.renderPass = renderPass; // NOTE - We only have one render pass for now, if that changes we must change it here too
		inheritanceInfo.subpass = 0;
		inheritanceInfo.framebuffer = GetSWIDDAtIndex(frameBufferIndex)->swapChainFramebuffer;

		commandBuffer.BeginRecording(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, &inheritanceInfo);

		commandBuffer.CMD_BindMesh(resources);
		commandBuffer.CMD_BindDescriptorSets(pipelineLayout, static_cast<uint32_t>(vkDescSets.size()), vkDescSets.data());
		commandBuffer.CMD_BindGraphicsPipeline(graphicsPipeline);
		commandBuffer.CMD_SetScissor({ 0, 0 }, swapChainExtent);
		commandBuffer.CMD_SetViewport(static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height));
		commandBuffer.CMD_DrawIndexed(static_cast<uint32_t>(resources->indexCount));

		commandBuffer.EndRecording();
	}

	void Renderer::RecreateSwapChain()
	{
		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(windowHandle, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(windowHandle, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(logicalDevice);

		CleanupSwapChain();

		CreateSwapChain();
		CreateImageViews();
		CreateColorResources();
		CreateDepthResources();
		CreateFramebuffers();
		RecreateAllSecondaryCommandBuffers();
	}

	void Renderer::RecreateAllSecondaryCommandBuffers()
	{
		for (uint32_t i = 0; i < GetSWIDDSize(); i++)
		{
			for (auto& resources : assetResources)
			{
				SecondaryCommandBuffer* commandBuffer = GetSecondaryCommandBufferAtIndex(i, resources.uuid);
				commandBuffer->Create(logicalDevice, commandPools[GRAPHICS_QUEUE]);
				RecordSecondaryCommandBuffer(*commandBuffer, &resources, i);
			}
		}
	}

	void Renderer::CleanupSwapChain()
	{
		vkDestroyImageView(logicalDevice, colorImageView, nullptr);
		vkDestroyImage(logicalDevice, colorImage, nullptr);
		vkFreeMemory(logicalDevice, colorImageMemory, nullptr);

		vkDestroyImageView(logicalDevice, depthImageView, nullptr);
		vkDestroyImage(logicalDevice, depthImage, nullptr);
		vkFreeMemory(logicalDevice, depthImageMemory, nullptr);

		for (auto& swidd : swapChainImageDependentData)
		{
			vkDestroyFramebuffer(logicalDevice, swidd.swapChainFramebuffer, nullptr);
		}

		for (auto& swidd : swapChainImageDependentData)
		{
			vkDestroyImageView(logicalDevice, swidd.swapChainImageView, nullptr);
		}

		// Clean up the secondary commands buffers that reference the swap chain framebuffers
		for (uint32_t i = 0; i < GetSWIDDSize(); i++)
		{
			auto& secondaryCmdBuffer = swapChainImageDependentData[i].secondaryCommandBuffer;
			for (auto iter : secondaryCmdBuffer)
			{
				iter.second.Destroy(logicalDevice, commandPools[GRAPHICS_QUEUE]);
			}
		}

		vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
	}

	void Renderer::UpdateInfrequentUniformBuffers(UUID uuid)
	{
		using namespace glm;

		float aspectRatio = swapChainExtent.width / static_cast<float>(swapChainExtent.height);

		// Construct the ViewProj UBO
		ViewProjUBO vpUBO;
		vpUBO.view = lookAt(cameraPos, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
		vpUBO.proj = perspective(radians(45.0f), aspectRatio, 0.1f, 1000.0f);

		// NOTE - GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted
		vpUBO.proj[1][1] *= -1;

		uint32_t fddSize = GetFDDSize();
		for (uint32 i = 0; i < fddSize; i++)
		{
			FrameDependentData* currentFDD = GetFDDAtIndex(i);
			auto& currentAssetDataMap = currentFDD->assetDescriptorDataMap[uuid];

			currentAssetDataMap.viewProjUBO.UpdateData(&vpUBO, sizeof(ViewProjUBO));
		}
	}

	void Renderer::UpdateInfrequentDescriptorSets(UUID uuid)
	{
		uint32_t fddSize = GetFDDSize();
		for (uint32_t i = 0; i < fddSize; i++)
		{
			FrameDependentData* currentFDD = GetFDDAtIndex(i);
			auto& currentAssetDataMap = currentFDD->assetDescriptorDataMap[uuid];

			// Update ViewProj / image sampler descriptor sets
			WriteDescriptorSets writeDescSets(1, 0);
			writeDescSets.AddUniformBuffer(currentAssetDataMap.descriptorSets[0].GetDescriptorSet(), 0, currentAssetDataMap.viewProjUBO.GetBuffer(), currentAssetDataMap.viewProjUBO.GetBufferSize(), 0);
			currentAssetDataMap.descriptorSets[0].Update(logicalDevice, writeDescSets);
		}
	}

	void Renderer::UpdatePerFrameUniformBuffers(const Transform& transform, UUID uuid)
	{
		// Construct and update the transform UBO
		TransformUBO tempUBO{};

		glm::mat4 finalTransform = glm::identity<glm::mat4>();

		glm::mat4 translation = glm::translate(glm::identity<glm::mat4>(), transform.position);
		glm::mat4 rotation = glm::eulerAngleXYZ(transform.rotation.x, transform.rotation.y, transform.rotation.z);
		glm::mat4 scale = glm::scale(glm::identity<glm::mat4>(), transform.scale);

		tempUBO.transform = translation * rotation * scale;
		GetCurrentFDD()->assetDescriptorDataMap[uuid].transformUBO.UpdateData(&tempUBO, sizeof(TransformUBO));

		// Update the CameraDataUBO
		CameraDataUBO cameraUBO{};
		cameraUBO.cameraPos = glm::vec4(cameraPos, 1.0f);

		GetCurrentFDD()->cameraDataUBO.UpdateData(&cameraUBO, sizeof(CameraDataUBO));
	}

	void Renderer::UpdatePerFrameDescriptorSets(UUID uuid)
	{
		FrameDependentData* currentFDD = GetCurrentFDD();
		auto& currentAssetDataMap = currentFDD->assetDescriptorDataMap[uuid];

		// Update transform + cameraData descriptor sets
		WriteDescriptorSets writeDescSets(2, 0);
		writeDescSets.AddUniformBuffer(currentAssetDataMap.descriptorSets[1].GetDescriptorSet(), 0, currentAssetDataMap.transformUBO.GetBuffer(), currentAssetDataMap.transformUBO.GetBufferSize(), 0);
		writeDescSets.AddUniformBuffer(currentAssetDataMap.descriptorSets[1].GetDescriptorSet(), 1, currentFDD->cameraDataUBO.GetBuffer(), currentFDD->cameraDataUBO.GetBufferSize(), 0);
		currentAssetDataMap.descriptorSets[1].Update(logicalDevice, writeDescSets);
	}

	VkCommandBuffer Renderer::BeginSingleTimeCommands(VkCommandPool pool)
	{
		VkResult res;

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = pool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		res = vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);
		if (res != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to allocate single-time command buffer!");
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		res = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		if (res != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to begin command buffer!");
		}

		return commandBuffer;
	}

	void Renderer::EndSingleTimeCommands(VkCommandBuffer commandBuffer, QueueType commandPoolType)
	{
		VkResult res;

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		res = vkQueueSubmit(queues[commandPoolType], 1, &submitInfo, VK_NULL_HANDLE);
		if (res != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to submit single-time command buffer!");
		}

		res = vkQueueWaitIdle(queues[commandPoolType]);
		if (res != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to wait until queue was idle when submitting single-time command buffer");
		}

		vkFreeCommandBuffers(logicalDevice, commandPools[commandPoolType], 1, &commandBuffer);
	}

	void Renderer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(commandPools[TRANSFER_QUEUE]);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = 0; // TODO
		barrier.dstAccessMask = 0; // TODO

		VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_NONE;
		VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_NONE;

		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (HasStencilComponent(format))
			{
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else
		{
			TNG_ASSERT_MSG(false, "Unsupported layout transition!");
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		EndSingleTimeCommands(commandBuffer, TRANSFER_QUEUE);
	}

	void Renderer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(commandPools[TRANSFER_QUEUE]);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		EndSingleTimeCommands(commandBuffer, TRANSFER_QUEUE);
	}

	VkFormat Renderer::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			{
				return format;
			}
		}

		throw std::runtime_error("Failed to find supported format!");
	}

	VkFormat Renderer::FindDepthFormat()
	{
		return FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	bool Renderer::HasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	void Renderer::GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
	{
		// Check if the texture format we want to use supports linear blitting
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		{
			TNG_ASSERT_MSG(false, "Texture image does not support linear blitting!");
		}

		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(commandPools[GRAPHICS_QUEUE]);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = texWidth;
		int32_t mipHeight = texHeight;

		for (uint32_t i = 1; i < mipLevels; i++)
		{

			// Transition image from transfer dst optimal to transfer src optimal, since we're reading from this image
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			// Transition image from src transfer optimal to shader read only
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		// Transfer the last mip level to shader read-only because this wasn't handled by the loop above
		// (since we didn't blit from the last image)
		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		EndSingleTimeCommands(commandBuffer, GRAPHICS_QUEUE);
	}

	VkSampleCountFlagBits Renderer::GetMaxUsableSampleCount()
	{
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

		VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
		if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
		if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
		if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
		if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
		if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
		if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

		return VK_SAMPLE_COUNT_1_BIT;
	}

	/////////////////////////////////////////////////////
	// 
	// Frame dependent data
	//
	/////////////////////////////////////////////////////
	Renderer::FrameDependentData* Renderer::GetCurrentFDD()
	{
		return &frameDependentData[currentFrame];
	}

	Renderer::FrameDependentData* Renderer::GetFDDAtIndex(uint32_t frameIndex)
	{
		TNG_ASSERT_MSG(frameIndex >= 0 && frameIndex < frameDependentData.size(), "Invalid index used to retrieve frame-dependent data");
		return &frameDependentData[frameIndex];
	}

	uint32_t Renderer::GetFDDSize() const
	{
		return static_cast<uint32_t>(frameDependentData.size());
	}

	/////////////////////////////////////////////////////
	// 
	// Swap-chain image dependent data
	//
	/////////////////////////////////////////////////////

	// Returns the swap-chain image dependent data at the provided index
	Renderer::SwapChainImageDependentData* Renderer::GetSWIDDAtIndex(uint32_t frameIndex)
	{
		TNG_ASSERT_MSG(frameIndex >= 0 && frameIndex < swapChainImageDependentData.size(), "Invalid index used to retrieve swap-chain image dependent data");
		return &swapChainImageDependentData[frameIndex];
	}

	uint32_t Renderer::GetSWIDDSize() const
	{
		return static_cast<uint32_t>(swapChainImageDependentData.size());
	}

}