
#include "renderer.h"

// DISABLE WARNINGS FROM GLM
// 4201: warning C4201: nonstandard extension used: nameless struct/union
// 4244: warning C4244: 'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning(push)
#pragma warning(disable : 4201 4244)

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/gtx/euler_angles.hpp>

#pragma warning(pop) 

#include <array>
#include <chrono>
#include <cstdlib>
#include <cstdint>

// Unfortunately the renderer has to know about GLFW in order to create the surface, since the vulkan call itself
// takes in a GLFWwindow pointer >:(. This also means we have to pass it into the renderer's Initialize() call,
// since the surface has to be initialized for other Vulkan objects to be properly initialized as well...
#include <glfw/glfw3.h> // GLFWwindow, glfwCreateWindowSurface() and glfwGetRequiredInstanceExtensions()

#include <glm/glm.hpp>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

#include "asset_loader.h"
#include "cmd_buffer/disposable_command.h"
#include "command_pool_registry.h"
#include "data_buffer/vertex_buffer.h"
#include "data_buffer/index_buffer.h"
#include "default_material.h"
#include "descriptors/write_descriptor_set.h"
#include "device_cache.h"
#include "queue_family_indices.h"
#include "utils/file_utils.h"

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

static const std::string SkyboxTextureFilePath = "../src/data/textures/cubemaps/skybox_sunny.hdr";

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

	struct ViewUBO
	{
		glm::mat4 view;
	};

	struct ProjUBO
	{
		glm::mat4 proj;
	};

	// The minimum uniform buffer alignment of the chosen physical device is 64 bytes...an entire matrix 4
	// 
	struct CameraDataUBO
	{
		glm::vec4 position;
		float exposure;
		char padding[44];
	};
	TNG_ASSERT_COMPILE(sizeof(CameraDataUBO) == 64);

	Renderer::Renderer() : 
		vkInstance(VK_NULL_HANDLE), debugMessenger(VK_NULL_HANDLE), surface(VK_NULL_HANDLE), queues(), swapChain(VK_NULL_HANDLE), 
		swapChainImageFormat(VK_FORMAT_UNDEFINED), swapChainExtent({ 0, 0 }), frameDependentData(), swapChainImageDependentData(),
		pbrSetLayoutCache(), pbrRenderPass(), pbrPipeline(), currentFrame(0), resourcesMap(), assetResources(), descriptorPool(),
		depthBuffer(), colorAttachment(), framebufferWidth(0), framebufferHeight(0), cubemapPreprocessingPipeline(), 
		cubemapPreprocessingRenderPass(), cubemapPreprocessingSetLayoutCache()
	{ }

	void Renderer::Initialize(GLFWwindow* windowHandle, uint32_t windowWidth, uint32_t windowHeight)
	{
		frameDependentData.resize(MAX_FRAMES_IN_FLIGHT);
		framebufferWidth = windowWidth;
		framebufferHeight = windowHeight;

		// Initialize Vulkan-related objects
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface(windowHandle);
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateDescriptorSetLayouts();
		CreateDescriptorPool();
		CreateRenderPasses();
		CreatePipelines();
		CreateCommandPools();
		LoadSkyboxResources();
		CreateColorAttachmentTexture();
		CreateDepthTexture();
		CreateFramebuffers();
		CreatePrimaryCommandBuffers(QueueType::GRAPHICS);
		CreateSyncObjects();

		// After initializing the renderer, let's do some pre-processing...
		CalculateSkyboxCubemap();
	}

	void Renderer::Update(float deltaTime)
	{
		UNUSED(deltaTime);

		if (swapChainExtent.width != framebufferWidth || swapChainExtent.height != framebufferHeight)
		{
			RecreateSwapChain();
		}
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

	void Renderer::Shutdown()
	{
		VkDevice logicalDevice = DeviceCache::Get().GetLogicalDevice();

		vkDeviceWaitIdle(logicalDevice);

		DestroyAllAssetResources();

		CleanupSwapChain();

		pbrSetLayoutCache.DestroyLayouts();
		cubemapPreprocessingSetLayoutCache.DestroyLayouts();

		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			auto frameData = GetFDDAtIndex(i);
			for (auto& iter : frameData->assetDescriptorDataMap)
			{
				iter.second.transformUBO.Destroy();
				iter.second.projUBO.Destroy();
				iter.second.viewUBO.Destroy();
				iter.second.cameraDataUBO.Destroy();
			}
		}

		descriptorPool.Destroy();

		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			auto frameData = GetFDDAtIndex(i);
			vkDestroySemaphore(logicalDevice, frameData->imageAvailableSemaphore, nullptr);
			vkDestroySemaphore(logicalDevice, frameData->renderFinishedSemaphore, nullptr);
			vkDestroyFence(logicalDevice, frameData->inFlightFence, nullptr);
		}

		CommandPoolRegistry::Get().DestroyPools();

		// Destroy skybox cubemap
		for (uint32_t i = 0; i < 6; i++)
		{
			cubemapFaces[i].Destroy();
		}

		pbrPipeline.Destroy();
		cubemapPreprocessingPipeline.Destroy();

		pbrRenderPass.Destroy();
		cubemapPreprocessingRenderPass.Destroy();

		vkDestroyDevice(logicalDevice, nullptr);
		DeviceCache::Get().InvalidateCache();

		vkDestroySurfaceKHR(vkInstance, surface, nullptr);

		if (enableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, nullptr);
		}

		vkDestroyInstance(vkInstance, nullptr);
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

		//////////////////////////////
		//
		//	MESH
		//
		//////////////////////////////
		for (uint32_t i = 0; i < meshCount; i++)
		{
			Mesh& currMesh = asset->meshes[i];

			// Create the vertex buffer
			uint64_t numBytes = currMesh.vertices.size() * sizeof(PBRVertex);

			VertexBuffer& vb = resources.vertexBuffers[i];
			vb.Create(numBytes);

			{
				DisposableCommand command(QueueType::TRANSFER);
				vb.CopyIntoBuffer(command.GetBuffer(), currMesh.vertices.data(), numBytes);
			}

			// Create the index buffer
			numBytes = currMesh.indices.size() * sizeof(IndexType);

			IndexBuffer& ib = resources.indexBuffer;
			ib.Create(numBytes);

			{
				DisposableCommand command(QueueType::TRANSFER);
				ib.CopyIntoBuffer(command.GetBuffer(), currMesh.indices.data(), numBytes);
			}

			// Destroy the staging buffers
			vb.DestroyIntermediateBuffers();
			ib.DestroyIntermediateBuffers();

			// Accumulate the index count of this mesh;
			totalIndexCount += currMesh.indices.size();

			// Set the current offset and then increment
			resources.offsets[i] = vBufferOffset++;
		}

		//////////////////////////////
		//
		//	MATERIAL
		//
		//////////////////////////////
		uint32_t numMaterials = static_cast<uint32_t>(asset->materials.size());
		TNG_ASSERT_MSG(numMaterials <= 1, "Multiple materials per asset are not currently supported!");

		if (numMaterials == 0)
		{
			// We need at least _one_ material, even if we didn't deserialize any material information
			// In this case we use a default material (look at default_material.h)
			asset->materials.resize(1);
		}
		Material& material = asset->materials[0];

		// Resize to the number of possible texture types
		resources.material.resize(static_cast<uint32_t>(Material::TEXTURE_TYPE::_COUNT));

		// Pre-emptively fill out the texture create info, so we can just pass it to all CreateFromFile() calls
		SamplerCreateInfo samplerInfo{};
		samplerInfo.minificationFilter = VK_FILTER_LINEAR;
		samplerInfo.magnificationFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.maxAnisotropy = 1.0f; // Is this an appropriate value??

		ImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.aspect = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;

		BaseImageCreateInfo baseImageInfo{};
		baseImageInfo.width = 0; // Unused
		baseImageInfo.height = 0; // Unused
		baseImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		baseImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		baseImageInfo.mipLevels = 0; // Unused
		baseImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

		for (uint32_t i = 0; i < static_cast<uint32_t>(Material::TEXTURE_TYPE::_COUNT); i++)
		{
			Material::TEXTURE_TYPE texType = static_cast<Material::TEXTURE_TYPE>(i);
			if (material.HasTextureOfType(texType))
			{
				Texture* matTexture = material.GetTextureOfType(texType);
				TNG_ASSERT_MSG(matTexture != nullptr, "Why is this texture nullptr when we specifically checked against it?");

				TextureResource& texResource = resources.material[i];
				texResource.CreateFromFile(matTexture->fileName, &baseImageInfo, &viewCreateInfo, &samplerInfo);
			}
			else
			{
				// Create a fallback for use in the shader
				BaseImageCreateInfo fallbackBaseImageInfo{};
				fallbackBaseImageInfo.width = 1;
				fallbackBaseImageInfo.height = 1;
				fallbackBaseImageInfo.mipLevels = 1;
				fallbackBaseImageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
				fallbackBaseImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				fallbackBaseImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

				uint32_t data = DEFAULT_MATERIAL.at(texType);

				TextureResource& texResource = resources.material[i];
				texResource.Create(&fallbackBaseImageInfo, &viewCreateInfo, &samplerInfo);
				texResource.CopyDataIntoImage(static_cast<void*>(&data), sizeof(data));
				texResource.TransitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}

		// Insert the asset's uuid into the assetDrawState map. We do not render it
		// upon insertion by default
		resources.shouldDraw = false;
		resources.transform = Transform();
		resources.indexCount = totalIndexCount;
		resources.uuid = asset->uuid;

		// Create uniform buffers for this asset
		CreateAssetUniformBuffers(resources.uuid);

		CreateDescriptorSets(resources.uuid);

		// Initialize the view + projection matrix UBOs to some values, so when new assets are created they get sensible defaults
		// for their descriptor sets. 
		// Note that we're operating under the assumption that assets will only be created before
		// we hit the update loop, simply because we're updating all frames in flight here. If this changes in the future, another
		// solution must be implemented
		glm::vec3 pos = { 0.0f, 5.0f, 15.0f };
		glm::vec3 eye = { 0.0f, 0.0f, 1.0f };
		glm::mat4 viewMat = glm::inverse(glm::lookAt(pos, pos + eye, { 0.0f, 1.0f, 0.0f }));  // TODO - Remove this hard-coded stuff
		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{

			UpdateCameraDataUniformBuffers(resources.uuid, i, pos, viewMat);
			UpdateProjectionUniformBuffer(resources.uuid, i);

			InitializeDescriptorSets(resources.uuid, i);
		}


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
			commandBuffer.Create(CommandPoolRegistry::Get().GetCommandPool(QueueType::GRAPHICS));
		}
	}

	void Renderer::DestroyAssetBuffersHelper(AssetResources* resources)
	{
		// Destroy all vertex buffers
		for (auto& vb : resources->vertexBuffers)
		{
			vb.Destroy();
		}

		// Destroy the index buffer
		resources->indexBuffer.Destroy();

		// Destroy textures
		for (auto& tex : resources->material)
		{
			tex.Destroy();
		}
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
		AssetResources* asset = GetAssetResourcesFromUUID(cubeMeshUUID);
		if (asset == nullptr)
		{
			LogError("Failed to find asset resources for asset with uuid %ull!", uuid);
			return;
		}

		// Destroy the resources
		DestroyAssetBuffersHelper(asset);

		// Remove resources from the vector
		uint32_t resourceIndex = static_cast<uint32_t>((asset - &assetResources[0]) / sizeof(assetResources));
		assetResources.erase(assetResources.begin() + resourceIndex);

		// Destroy reference to resources
		resourcesMap.erase(uuid);
	}

	void Renderer::DestroyAllAssetResources()
	{
		uint32_t numAssetResources = static_cast<uint32_t>(assetResources.size());

		for (uint32_t i = 0; i < numAssetResources; i++)
		{
			DestroyAssetBuffersHelper(&assetResources[i]);
		}

		assetResources.clear();
		resourcesMap.clear();
	}

	void Renderer::CreateSurface(GLFWwindow* windowHandle)
	{
		if (glfwCreateWindowSurface(vkInstance, windowHandle, nullptr, &surface) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create window surface!");
		}
	}

	void Renderer::SetNextFramebufferSize(uint32_t newWidth, uint32_t newHeight)
	{
		framebufferWidth = newWidth;
		framebufferHeight = newHeight;
	}

	void Renderer::UpdateCameraData(const glm::vec3& position, const glm::mat4& viewMatrix)
	{
		FrameDependentData* currentFDD = GetCurrentFDD();
		auto& assetDescriptorMap = currentFDD->assetDescriptorDataMap;

		// Update the view matrix and camera position UBOs for all assets, as well as the descriptor sets
		for (auto& assetData : assetDescriptorMap)
		{
			UpdateCameraDataUniformBuffers(assetData.first, currentFrame, position, viewMatrix);
			UpdateCameraDataDescriptorSet(assetData.first, currentFrame);
		}
	}

	void Renderer::RecreateSwapChain()
	{
		VkDevice logicalDevice = GetLogicalDevice();

		vkDeviceWaitIdle(logicalDevice);

		CleanupSwapChain();

		CreateSwapChain();
		CreateColorAttachmentTexture();
		CreateDepthTexture();
		CreateFramebuffers();
		RecreateAllSecondaryCommandBuffers();
	}

	void Renderer::SetAssetDrawState(UUID uuid)
	{
		AssetResources* resources = GetAssetResourcesFromUUID(uuid);
		if (resources == nullptr)
		{
			// Maybe the asset resources were deleted but we somehow forgot to remove it from the assetDrawStates map?
			LogError(false, "Attempted to set asset draw state, but asset does not exist or UUID is invalid!");
			return;
		}

		resources->shouldDraw = true;
	}

	void Renderer::SetAssetTransform(UUID uuid, const Transform& transform)
	{
		AssetResources* asset = GetAssetResourcesFromUUID(uuid);
		if (asset == nullptr)
		{
			return;
		}

		asset->transform = transform;
	}

	void Renderer::SetAssetPosition(UUID uuid, const glm::vec3& position)
	{
		AssetResources* asset = GetAssetResourcesFromUUID(uuid);
		if (asset == nullptr)
		{
			return;
		}

		Transform& transform = asset->transform;
		transform.position = position;
	}

	void Renderer::SetAssetRotation(UUID uuid, const glm::vec3& rotation)
	{
		AssetResources* asset = GetAssetResourcesFromUUID(uuid);
		if (asset == nullptr)
		{
			return;
		}

		Transform& transform = asset->transform;
		transform.rotation = rotation;
	}

	void Renderer::SetAssetScale(UUID uuid, const glm::vec3& scale)
	{
		AssetResources* asset = GetAssetResourcesFromUUID(uuid);
		if (asset == nullptr)
		{
			return;
		}

		Transform& transform = asset->transform;
		transform.scale = scale;
	}

	void Renderer::DrawFrame()
	{
		VkDevice logicalDevice = GetLogicalDevice();
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

		if (SubmitQueue(QueueType::GRAPHICS, &submitInfo, 1, currentFDD->inFlightFence) != VK_SUCCESS)
		{
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

		result = vkQueuePresentKHR(queues[QueueType::PRESENT], &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
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
			LogError("Validation layers were requested, but one or more is not supported!");
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
		createInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | */VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
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
			TNG_ASSERT_MSG(false, "Failed to setup debug messenger!");
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
				DeviceCache::Get().CachePhysicalDevice(device);
				return;
			}
		}

		TNG_ASSERT_MSG(false, "Failed to find a suitable physical device!");
	}

	bool Renderer::IsDeviceSuitable(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices = FindQueueFamilies(device, surface);
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

	VkExtent2D Renderer::ChooseSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t actualWidth, uint32_t actualHeight)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		else
		{
			VkExtent2D actualExtent =
			{
				static_cast<uint32_t>(actualWidth),
				static_cast<uint32_t>(actualHeight)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
				capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
				capabilities.maxImageExtent.height);

			return actualExtent;

		}
	}

	void Renderer::CreateLogicalDevice()
	{
		VkPhysicalDevice physicalDevice = GetPhysicalDevice();

		QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);
		if (!indices.IsComplete())
		{
			LogError("Failed to create logical device because the queue family indices are incomplete!");
		}

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = {
			indices.GetIndex(QueueType::GRAPHICS),
			indices.GetIndex(QueueType::PRESENT),
			indices.GetIndex(QueueType::TRANSFER)
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
			// We get a warning about using deprecated and ignored 'ppEnabledLayerNames', so I've commented these out.
			// It looks like validation layers work regardless...somehow...
			//createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			//createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		VkDevice device;
		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create the logical device!");
		}

		DeviceCache::Get().CacheLogicalDevice(device);

		// Get the queues from the logical device
		vkGetDeviceQueue(device, indices.GetIndex(QueueType::GRAPHICS), 0, &queues[QueueType::GRAPHICS]);
		vkGetDeviceQueue(device, indices.GetIndex(QueueType::PRESENT), 0, &queues[QueueType::PRESENT]);
		vkGetDeviceQueue(device, indices.GetIndex(QueueType::TRANSFER), 0, &queues[QueueType::TRANSFER]);

	}

	void Renderer::CreateSwapChain()
	{
		VkDevice logicalDevice = GetLogicalDevice();
		VkPhysicalDevice physicalDevice = GetPhysicalDevice();

		SwapChainSupportDetails details = QuerySwapChainSupport(physicalDevice);
		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(details.formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(details.presentModes);
		VkExtent2D extent = ChooseSwapChainExtent(details.capabilities, framebufferWidth, framebufferHeight);

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

		QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);
		uint32_t queueFamilyIndices[2] = { indices.GetIndex(QueueType::GRAPHICS), indices.GetIndex(QueueType::PRESENT)};

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

		// Get the number of images, then we use the count to create the image views below
		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;

		CreateSwapChainImageViews(imageCount);
	}

	// Create image views for all images on the swap chain
	void Renderer::CreateSwapChainImageViews(uint32_t imageCount)
	{
		VkDevice logicalDevice = GetLogicalDevice();

		swapChainImageDependentData.resize(imageCount);
		std::vector<VkImage> swapChainImages(imageCount);

		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());

		for (uint32_t i = 0; i < imageCount; i++)
		{
			swapChainImageDependentData[i].swapChainImage.CreateImageViewFromBase(swapChainImages[i], swapChainImageFormat, 1, VK_IMAGE_ASPECT_COLOR_BIT);
		}
		swapChainImages.clear();
	}

	void Renderer::CreateCommandPools()
	{
		CommandPoolRegistry::Get().CreatePools(surface);
	}

	void Renderer::CreatePipelines()
	{
		cubemapPreprocessingPipeline.SetData(cubemapPreprocessingRenderPass, cubemapPreprocessingSetLayoutCache, swapChainExtent);
		cubemapPreprocessingPipeline.Create();

		pbrPipeline.SetData(pbrRenderPass, pbrSetLayoutCache, swapChainExtent);
		pbrPipeline.Create();
	}

	void Renderer::CreateRenderPasses()
	{
		// Some terminology about render passes from Reddit thread (https://www.reddit.com/r/vulkan/comments/a27cid/what_is_an_attachment_in_the_render_passes/):
		//
		// An image is just a piece of memory with some metadata about layout, format etc. A Framebuffer is a container for multiple images 
		// with additional metadata for each image, like usage, identifier(index) and type(color, depth, etc.). 
		// These images used in a framebuffer are called attachments, because they are attached to, and owned by, a framebuffer.
		// 
		// Attachments that get rendered to are called Render Targets, Attachments which are used as input are called Input Attachments.
		// 
		// Attachments which hold information about Multisampling are called Resolve Attachments.
		// 
		// Attachments with RGB/Depth/Stencil information are called Color/Depth/Stencil Attachments respectively.

		cubemapPreprocessingRenderPass.Create();

		pbrRenderPass.SetData(swapChainImageFormat, FindDepthFormat());
		pbrRenderPass.Create();
	}

	void Renderer::CreateFramebuffers()
	{
		CreatePBRFramebuffer();
		CreateCubemapPreprocessingFramebuffer();
	}

	void Renderer::CreatePBRFramebuffer()
	{
		auto& swidd = swapChainImageDependentData;

		for (size_t i = 0; i < GetSWIDDSize(); i++)
		{
			std::array<VkImageView, 3> attachments =
			{
				colorAttachment.GetImageView(),
				depthBuffer.GetImageView(),
				swidd[i].swapChainImage.GetImageView()
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = pbrRenderPass.GetRenderPass();
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(GetLogicalDevice(), &framebufferInfo, nullptr, &(swidd[i].swapChainFramebuffer)) != VK_SUCCESS)
			{
				TNG_ASSERT_MSG(false, "Failed to create PBR framebuffer!");
			}
		}
	}

	void Renderer::CreateCubemapPreprocessingFramebuffer()
	{
		// Do we need to make 6 framebuffers?
		std::array<VkImageView, 1> attachments =
		{
			cubemapFaces[0].GetImageView()
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = cubemapPreprocessingRenderPass.GetRenderPass();
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = framebufferWidth;
		framebufferInfo.height = framebufferHeight;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(GetLogicalDevice(), &framebufferInfo, nullptr, &cubemapPreprocessingFramebuffer) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create cubemap preprocessing framebuffer!");
		}
	}

	void Renderer::CreatePrimaryCommandBuffers(QueueType poolType)
	{
		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			GetFDDAtIndex(i)->primaryCommandBuffer.Create(CommandPoolRegistry::Get().GetCommandPool(poolType));
		}
	}

	void Renderer::CreateSyncObjects()
	{
		VkDevice logicalDevice = GetLogicalDevice();

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

	void Renderer::CreateAssetUniformBuffers(UUID uuid)
	{
		VkDeviceSize transformUBOSize = sizeof(TransformUBO);
		VkDeviceSize viewUBOSize = sizeof(ViewUBO);
		VkDeviceSize projUBOSize = sizeof(ProjUBO);
		VkDeviceSize cameraDataSize = sizeof(CameraDataUBO);

		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			FrameDependentData* currentFDD = GetFDDAtIndex(i);
			AssetDescriptorData& assetDescriptorData = currentFDD->assetDescriptorDataMap[uuid];

			// Create the TransformUBO
			UniformBuffer& transUBO = assetDescriptorData.transformUBO;
			transUBO.Create(transformUBOSize);
			transUBO.MapMemory(transformUBOSize);
				
			// Create the ViewUBO
			UniformBuffer& vpUBO = assetDescriptorData.viewUBO;
			vpUBO.Create(viewUBOSize);
			vpUBO.MapMemory(viewUBOSize);

			// Create the ProjUBO
			UniformBuffer& projUBO = assetDescriptorData.projUBO;
			projUBO.Create(projUBOSize);
			projUBO.MapMemory(projUBOSize);

			// Create the camera data UBO
			UniformBuffer& cameraDataUBO = assetDescriptorData.cameraDataUBO;
			cameraDataUBO.Create(cameraDataSize);
			cameraDataUBO.MapMemory(cameraDataSize);
		}
	}

	void Renderer::CreateDescriptorSetLayouts()
	{
		CreateCubemapPreprocessingSetLayouts();
		CreatePBRSetLayouts();
	}

	void Renderer::CreatePBRSetLayouts()
	{
		//	DIFFUSE = 0,
		//	NORMAL,
		//	METALLIC,
		//	ROUGHNESS,
		//	LIGHTMAP,

		// Holds PBR textures
		SetLayoutSummary persistentLayout;
		persistentLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // Diffuse texture
		persistentLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // Normal texture
		persistentLayout.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // Metallic texture
		persistentLayout.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // Roughness texture
		persistentLayout.AddBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // Lightmap texture
		pbrSetLayoutCache.CreateSetLayout(persistentLayout, 0);

		// Holds ProjUBO
		SetLayoutSummary unstableLayout;
		unstableLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);           // Projection matrix
		unstableLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);         // Camera exposure
		pbrSetLayoutCache.CreateSetLayout(unstableLayout, 0);

		// Holds TransformUBO + ViewUBO + CameraDataUBO
		SetLayoutSummary volatileLayout;
		volatileLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);   // Transform matrix
		volatileLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT); // Camera data
		volatileLayout.AddBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);   // View matrix
		pbrSetLayoutCache.CreateSetLayout(volatileLayout, 0);
	}

	void Renderer::CreateCubemapPreprocessingSetLayouts()
	{
		// NOTE - We're only separating the view/proj matrices because we can re-use the structs that the PBR view/proj uniforms use
		SetLayoutSummary volatileLayout;
		volatileLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // View matrix
		volatileLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // Projection matrix
		volatileLayout.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // Equirectangular map

		cubemapPreprocessingSetLayoutCache.CreateSetLayout(volatileLayout, 0);
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

		const uint32_t numUniformBuffers = 4;
		const uint32_t numImageSamplers = 5;

		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = numUniformBuffers * GetFDDSize();
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = numImageSamplers * GetFDDSize();

		descriptorPool.Create(poolSizes.data(), static_cast<uint32_t>(poolSizes.size()), static_cast<uint32_t>(poolSizes.size()) * fddSize * MAX_ASSET_COUNT, 0);
	}

	void Renderer::CreateDescriptorSets(UUID uuid)
	{
		uint32_t fddSize = GetFDDSize();

		for (uint32_t i = 0; i < fddSize; i++)
		{
			FrameDependentData* currentFDD = GetFDDAtIndex(i);
			currentFDD->assetDescriptorDataMap.insert({uuid, AssetDescriptorData()});
			AssetDescriptorData& assetDescriptorData = currentFDD->assetDescriptorDataMap[uuid];

			const LayoutCache& cache = pbrSetLayoutCache.GetLayoutCache();
			for (auto iter : cache)
			{
				assetDescriptorData.descriptorSets.push_back(DescriptorSet());
				DescriptorSet* currentSet = &assetDescriptorData.descriptorSets.back();

				currentSet->Create(descriptorPool, iter.second);
			}
		}

	}

	void Renderer::CreateDepthTexture()
	{
		VkFormat depthFormat = FindDepthFormat();

		// Base image
		BaseImageCreateInfo imageInfo{};
		imageInfo.width = framebufferWidth;
		imageInfo.height = framebufferHeight;
		imageInfo.format = depthFormat;
		imageInfo.format = depthFormat;
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imageInfo.mipLevels = 1;
		imageInfo.samples = DeviceCache::Get().GetMaxMSAA();

		// Image view
		ImageViewCreateInfo imageViewInfo{ VK_IMAGE_ASPECT_DEPTH_BIT };

		depthBuffer.Create(&imageInfo, &imageViewInfo);
	}

	void Renderer::CreateColorAttachmentTexture()
	{
		// Base image
		BaseImageCreateInfo imageInfo{};
		imageInfo.width = framebufferWidth;
		imageInfo.height = framebufferHeight;
		imageInfo.format = swapChainImageFormat;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		imageInfo.mipLevels = 1;
		imageInfo.samples = DeviceCache::Get().GetMaxMSAA();

		// Image view
		ImageViewCreateInfo imageViewInfo{ VK_IMAGE_ASPECT_COLOR_BIT };

		colorAttachment.Create(&imageInfo, &imageViewInfo);
	}

	void Renderer::LoadSkyboxResources()
	{
		//
		// Load the skybox texture from file
		//

		BaseImageCreateInfo baseImageInfo{};
		baseImageInfo.width = 0; // Unused
		baseImageInfo.height = 0; // Unused
		baseImageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		baseImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		baseImageInfo.mipLevels = 0; // Unused
		baseImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

		ImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.aspect = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;

		SamplerCreateInfo samplerInfo{};
		samplerInfo.minificationFilter = VK_FILTER_LINEAR;
		samplerInfo.magnificationFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.maxAnisotropy = 1.0f; // Is this an appropriate value??

		skyboxTexture.CreateFromFile(SkyboxTextureFilePath, &baseImageInfo, &viewCreateInfo, &samplerInfo);

		//
		// Create the offscreen textures that we'll use to render the cube faces to
		//

		baseImageInfo.width = 512; // Doing 512x512 for now
		baseImageInfo.height = 512;
		baseImageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		baseImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		baseImageInfo.mipLevels = 0; // Unused?
		baseImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

		viewCreateInfo.aspect = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;

		samplerInfo.minificationFilter = VK_FILTER_LINEAR;
		samplerInfo.magnificationFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.maxAnisotropy = 1.0f; // Is this an appropriate value??

		for (uint32_t i = 0; i < 6; i++)
		{
			cubemapFaces[i].Create(&baseImageInfo, &viewCreateInfo, &samplerInfo);
		}

		//
		// Load the skybox mesh (unit cube) from file
		//


	}

	void Renderer::RecordPrimaryCommandBuffer(uint32_t frameBufferIndex)
	{
		PrimaryCommandBuffer* commandBuffer = GetCurrentPrimaryBuffer();

		// Reset the primary buffer since we've marked it as one-time submit
		commandBuffer->Reset();

		// Primary command buffers don't need to define inheritance info
		commandBuffer->BeginRecording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);

		commandBuffer->CMD_BeginRenderPass(pbrRenderPass.GetRenderPass(), GetFramebufferAtIndex(frameBufferIndex), swapChainExtent, true);

		// Execute the secondary commands here
		std::vector<VkCommandBuffer> secondaryCmdBuffers;
		secondaryCmdBuffers.resize(assetResources.size()); // At most we can have the same number of cmd buffers as there are asset resources
		uint32_t secondaryCmdBufferCount = 0;
		for (auto& iter : assetResources)
		{
			UUID& uuid = iter.uuid;

			if (iter.shouldDraw)
			{
				SecondaryCommandBuffer* secondaryCmdBuffer = GetSecondaryCommandBufferAtIndex(frameBufferIndex, uuid);

				UpdateTransformUniformBuffer(iter.transform, uuid);
				UpdateTransformDescriptorSet(uuid);

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
		inheritanceInfo.renderPass = pbrRenderPass.GetRenderPass();
		inheritanceInfo.subpass = 0;
		inheritanceInfo.framebuffer = GetSWIDDAtIndex(frameBufferIndex)->swapChainFramebuffer;

		commandBuffer.BeginRecording(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, &inheritanceInfo);

		commandBuffer.CMD_BindMesh(resources);
		commandBuffer.CMD_BindDescriptorSets(pbrPipeline.GetPipelineLayout(), static_cast<uint32_t>(vkDescSets.size()), vkDescSets.data());
		commandBuffer.CMD_BindGraphicsPipeline(pbrPipeline.GetPipeline());
		commandBuffer.CMD_SetScissor({ 0, 0 }, swapChainExtent);
		commandBuffer.CMD_SetViewport(static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height));
		commandBuffer.CMD_DrawIndexed(static_cast<uint32_t>(resources->indexCount));

		commandBuffer.EndRecording();
	}

	void Renderer::RecreateAllSecondaryCommandBuffers()
	{
		for (uint32_t i = 0; i < GetSWIDDSize(); i++)
		{
			for (auto& resources : assetResources)
			{
				SecondaryCommandBuffer* commandBuffer = GetSecondaryCommandBufferAtIndex(i, resources.uuid);
				commandBuffer->Create(CommandPoolRegistry::Get().GetCommandPool(QueueType::GRAPHICS));
				RecordSecondaryCommandBuffer(*commandBuffer, &resources, i);
			}
		}
	}

	void Renderer::CleanupSwapChain()
	{
		VkDevice logicalDevice = GetLogicalDevice();

		colorAttachment.Destroy();
		depthBuffer.Destroy();

		for (auto& swidd : swapChainImageDependentData)
		{
			vkDestroyFramebuffer(logicalDevice, swidd.swapChainFramebuffer, nullptr);
		}

		for (auto& swidd : swapChainImageDependentData)
		{
			swidd.swapChainImage.DestroyImageView();
		}

		// Clean up the secondary commands buffers that reference the swap chain framebuffers
		for (uint32_t i = 0; i < GetSWIDDSize(); i++)
		{
			auto& secondaryCmdBuffer = swapChainImageDependentData[i].secondaryCommandBuffer;
			for (auto iter : secondaryCmdBuffer)
			{
				iter.second.Destroy(CommandPoolRegistry::Get().GetCommandPool(QueueType::GRAPHICS));
			}
		}

		vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
	}

	void Renderer::UpdateProjectionUniformBuffer(UUID uuid, uint32_t frameIndex)
	{
		using namespace glm;

		float aspectRatio = swapChainExtent.width / static_cast<float>(swapChainExtent.height);

		// Construct the ProjUBO
		ProjUBO projUBO;
		projUBO.proj = perspective(radians(45.0f), aspectRatio, 0.1f, 1000.0f);

		// NOTE - GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted
		projUBO.proj[1][1] *= -1;

		FrameDependentData* currentFDD = GetFDDAtIndex(frameIndex);
		auto& currentAssetDataMap = currentFDD->assetDescriptorDataMap[uuid];

		currentAssetDataMap.projUBO.UpdateData(&projUBO, sizeof(ProjUBO));
	}

	void Renderer::UpdateProjectionDescriptorSet(UUID uuid, uint32_t frameIndex)
	{
		FrameDependentData* currentFDD = GetFDDAtIndex(frameIndex);
		auto& currentAssetDataMap = currentFDD->assetDescriptorDataMap[uuid];

		DescriptorSet& descSet = currentAssetDataMap.descriptorSets[1];

		// Update ProjUBO descriptor set
		WriteDescriptorSets writeDescSets(1, 0);
		writeDescSets.AddUniformBuffer(descSet.GetDescriptorSet(), 0, currentAssetDataMap.projUBO.GetBuffer(), currentAssetDataMap.projUBO.GetBufferSize(), 0);
		descSet.Update(writeDescSets);
	}

	void Renderer::UpdatePBRTextureDescriptorSet(UUID uuid, uint32_t frameIndex)
	{
		FrameDependentData* currentFDD = GetFDDAtIndex(frameIndex);
		auto& currentAssetDataMap = currentFDD->assetDescriptorDataMap[uuid];

		DescriptorSet& descSet = currentAssetDataMap.descriptorSets[0];

		// Get the asset resources so we can retrieve the textures
		AssetResources* asset = GetAssetResourcesFromUUID(cubeMeshUUID);
		if (asset == nullptr)
		{
			return;
		}

		// Update PBR textures
		WriteDescriptorSets writeDescSets(0, 5);
		writeDescSets.AddImageSampler(descSet.GetDescriptorSet(), 0, asset->material[static_cast<uint32_t>(Material::TEXTURE_TYPE::DIFFUSE)]);
		writeDescSets.AddImageSampler(descSet.GetDescriptorSet(), 1, asset->material[static_cast<uint32_t>(Material::TEXTURE_TYPE::NORMAL)]);
		writeDescSets.AddImageSampler(descSet.GetDescriptorSet(), 2, asset->material[static_cast<uint32_t>(Material::TEXTURE_TYPE::METALLIC)]);
		writeDescSets.AddImageSampler(descSet.GetDescriptorSet(), 3, asset->material[static_cast<uint32_t>(Material::TEXTURE_TYPE::ROUGHNESS)]);
		writeDescSets.AddImageSampler(descSet.GetDescriptorSet(), 4, asset->material[static_cast<uint32_t>(Material::TEXTURE_TYPE::LIGHTMAP)]);

		descSet.Update(writeDescSets);
	}

	void Renderer::UpdateCameraDataDescriptorSet(UUID uuid, uint32_t frameIndex)
	{
		FrameDependentData* currentFDD = GetFDDAtIndex(frameIndex);
		auto& currentAssetDataMap = currentFDD->assetDescriptorDataMap[uuid];

		DescriptorSet& descSet = currentAssetDataMap.descriptorSets[2];

		// Update view matrix + camera data descriptor set
		WriteDescriptorSets writeDescSets(2, 0);
		writeDescSets.AddUniformBuffer(descSet.GetDescriptorSet(), 2, currentAssetDataMap.viewUBO.GetBuffer(), currentAssetDataMap.viewUBO.GetBufferSize(), 0);
		writeDescSets.AddUniformBuffer(descSet.GetDescriptorSet(), 0, currentAssetDataMap.cameraDataUBO.GetBuffer(), currentAssetDataMap.cameraDataUBO.GetBufferSize(), 0);
		descSet.Update(writeDescSets);
	}

	void Renderer::UpdateTransformUniformBuffer(const Transform& transform, UUID uuid)
	{
		// Construct and update the transform UBO
		TransformUBO tempUBO{};

		glm::mat4 finalTransform = glm::identity<glm::mat4>();

		glm::mat4 translation = glm::translate(glm::identity<glm::mat4>(), transform.position);
		glm::mat4 rotation = glm::eulerAngleXYZ(transform.rotation.x, transform.rotation.y, transform.rotation.z);
		glm::mat4 scale = glm::scale(glm::identity<glm::mat4>(), transform.scale);

		tempUBO.transform = translation * rotation * scale;
		GetCurrentFDD()->assetDescriptorDataMap[uuid].transformUBO.UpdateData(&tempUBO, sizeof(TransformUBO));
	}

	void Renderer::UpdateCameraDataUniformBuffers(UUID uuid, uint32_t frameIndex, const glm::vec3& position, const glm::mat4& viewMatrix)
	{
		FrameDependentData* currentFDD = GetFDDAtIndex(frameIndex);
		auto& assetDescriptorData = currentFDD->assetDescriptorDataMap[uuid];

		ViewUBO viewUBO{};
		viewUBO.view = viewMatrix;
		assetDescriptorData.viewUBO.UpdateData(&viewUBO, sizeof(ViewUBO));

		CameraDataUBO cameraDataUBO{};
		cameraDataUBO.position = glm::vec4(position, 1.0f);
		cameraDataUBO.exposure = 1.0f;
		assetDescriptorData.cameraDataUBO.UpdateData(&cameraDataUBO, sizeof(CameraDataUBO));
	}

	void Renderer::UpdateTransformDescriptorSet(UUID uuid)
	{
		FrameDependentData* currentFDD = GetCurrentFDD();
		auto& currentAssetDataMap = currentFDD->assetDescriptorDataMap[uuid];

		DescriptorSet& descSet = currentAssetDataMap.descriptorSets[2];

		// Update transform + cameraData descriptor sets
		WriteDescriptorSets writeDescSets(2, 0);
		writeDescSets.AddUniformBuffer(descSet.GetDescriptorSet(), 0, currentAssetDataMap.transformUBO.GetBuffer(), currentAssetDataMap.transformUBO.GetBufferSize(), 0);
		writeDescSets.AddUniformBuffer(descSet.GetDescriptorSet(), 1, currentAssetDataMap.cameraDataUBO.GetBuffer(), currentAssetDataMap.cameraDataUBO.GetBufferSize(), 0);
		descSet.Update(writeDescSets);
	}

	void Renderer::InitializeDescriptorSets(UUID uuid, uint32_t frameIndex)
	{
		// Update all descriptor sets
		UpdateCameraDataDescriptorSet(uuid, frameIndex);
		UpdateProjectionDescriptorSet(uuid, frameIndex);
		UpdatePBRTextureDescriptorSet(uuid, frameIndex);
	}

	void Renderer::CalculateSkyboxCubemap()
	{
		AssetResources* asset = GetAssetResourcesFromUUID(cubeMeshUUID);
		if (asset == nullptr)
		{
			LogError("Failed to calculate the skybox cubemap, for some reason the cube mesh is not loaded!");
			return;
		}

		// Load the cube mesh somewhere else, and use it in this function
		TNG_TODO();

		PrimaryCommandBuffer cmdBuffer;
		cmdBuffer.Create(CommandPoolRegistry::Get().GetCommandPool(QueueType::GRAPHICS));
		cmdBuffer.BeginRecording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);

		cmdBuffer.CMD_BeginRenderPass(cubemapPreprocessingRenderPass.GetRenderPass(), cubemapPreprocessingFramebuffer, swapChainExtent, false);
		cmdBuffer.CMD_BindGraphicsPipeline(cubemapPreprocessingPipeline.GetPipeline());
		cmdBuffer.CMD_SetScissor({ 0, 0 }, swapChainExtent);
		cmdBuffer.CMD_SetViewport(static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height));
		cmdBuffer.CMD_BindMesh(asset);

		// For every face of the cube, we must change our camera's view direction, change the framebuffer (since we're rendering to
		// a different texture each pass)
		for (uint32_t i = 0; i < 6; i++)
		{
			cmdBuffer.CMD_BindDescriptorSets(cubemapPreprocessingPipeline.GetPipelineLayout(), static_cast<uint32_t>(vkDescSets.size()), vkDescSets.data());
			cmdBuffer.CMD_DrawIndexed(static_cast<uint32_t>(resources->indexCount));
		}

		cmdBuffer.CMD_EndRenderPass();

		cmdBuffer.EndRecording();

		VkCommandBuffer commandBuffers[1] = { cmdBuffer.GetBuffer() };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = 0;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = commandBuffers;

		if (SubmitQueue(QueueType::GRAPHICS, &submitInfo, 1, currentFDD->inFlightFence, true) != VK_SUCCESS)
		{
			return;
		}

		// Now that the equirectangular texture has been mapped to a cubemap, we don't need the original texture anymore
		skyboxTexture.Destroy();
	}

	VkResult Renderer::SubmitQueue(QueueType type, VkSubmitInfo* info, uint32_t submitCount, VkFence fence, bool waitUntilIdle)
	{
		VkResult res;
		VkQueue queue = queues[type];

		res = vkQueueSubmit(queue, submitCount, info, fence);
		if (res != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to submit queue!");
		}

		if (waitUntilIdle)
		{
			res = vkQueueWaitIdle(queue);
			if (res != VK_SUCCESS)
			{
				TNG_ASSERT_MSG(false, "Failed to wait until queue was idle after submitting!");
			}
		}

		return res;
	}

	AssetResources* Renderer::GetAssetResourcesFromUUID(UUID uuid)
	{
		auto resourceIndexIter = resourcesMap.find(uuid);
		if (resourceIndexIter == resourcesMap.end())
		{
			return nullptr;
		}

		uint32_t resourceIndex = resourceIndexIter->second;
		if (resourceIndex >= assetResources.size())
		{
			return nullptr;
		}

		return &assetResources[resourceIndex];
	}

	void Renderer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		DisposableCommand command(QueueType::TRANSFER);

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

		vkCmdCopyBufferToImage(command.GetBuffer(), buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}

	VkFormat Renderer::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		VkPhysicalDevice physicalDevice = GetPhysicalDevice();
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

		TNG_ASSERT_MSG(false, "Failed to find supported format!");
		return VK_FORMAT_UNDEFINED;
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