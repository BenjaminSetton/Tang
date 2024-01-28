
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
#include <set>
#include <unordered_map>
#include <vector>

#include "asset_loader.h"
#include "cmd_buffer/disposable_command.h"
#include "command_pool_registry.h"
#include "config.h"
#include "data_buffer/vertex_buffer.h"
#include "data_buffer/index_buffer.h"
#include "default_material.h"
#include "descriptors/write_descriptor_set.h"
#include "device_cache.h"
#include "queue_family_indices.h"
#include "utils/file_utils.h"
#include "ubo_structs.h"

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

	Renderer::Renderer() : 
		vkInstance(VK_NULL_HANDLE), debugMessenger(VK_NULL_HANDLE), surface(VK_NULL_HANDLE), queues(), swapChain(VK_NULL_HANDLE), 
		swapChainImageFormat(VK_FORMAT_UNDEFINED), swapChainExtent({ 0, 0 }), frameDependentData(), swapChainImageDependentData(),
		pbrSetLayoutCache(), pbrPipeline(), currentFrame(0), resourcesMap(), assetResources(), descriptorPool(),
		colorAttachment(), framebufferWidth(0), framebufferHeight(0), skyboxAssetUUID(INVALID_UUID),
		fullscreenQuadAssetUUID(INVALID_UUID)
	{ }

	void Renderer::Initialize(GLFWwindow* windowHandle, uint32_t windowWidth, uint32_t windowHeight)
	{
		frameDependentData.resize(CONFIG::MaxFramesInFlight);
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
		CreateColorAttachmentTextures();
		CreateDepthTextures();
		CreateFramebuffers();
		CreatePrimaryCommandBuffers();
		CreateSyncObjects();

		cubemapPreprocessingPass.LoadTextureResources();

		// Calculate the starting view direction and position of the camera
		glm::vec3 eye = { 0.0f, 0.0f, 1.0f };
		startingCameraPosition = { 0.0f, 5.0f, 15.0f };
		startingCameraViewMatrix = glm::inverse(glm::lookAt(startingCameraPosition, startingCameraPosition + eye, { 0.0f, 1.0f, 0.0f })); 

		// Calculate the starting projection matrix
		float aspectRatio = swapChainExtent.width / static_cast<float>(swapChainExtent.height);
		startingProjectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);
		startingProjectionMatrix[1][1] *= -1; // NOTE - GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted
	
		CreateFrameUniformBuffers();
		InitializeFrameUniformBuffers();
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

		currentFrame = (currentFrame + 1) % CONFIG::MaxFramesInFlight;
	}

	void Renderer::Shutdown()
	{
		VkDevice logicalDevice = DeviceCache::Get().GetLogicalDevice();

		vkDeviceWaitIdle(logicalDevice);

		DestroyAllAssetResources();

		CleanupSwapChain();

		cubemapPreprocessingPass.Destroy();
		skyboxPass.Destroy();

		ldrSetLayoutCache.DestroyLayouts();
		pbrSetLayoutCache.DestroyLayouts();

		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			auto frameData = GetFDDAtIndex(i);

			for (auto& iter : frameData->assetDescriptorDataMap)
			{
				iter.second.transformUBO.Destroy();
			}

			frameData->hdrFramebuffer.Destroy();

			frameData->ldrCameraDataUBO.Destroy();
			frameData->cameraDataUBO.Destroy();
			frameData->viewUBO.Destroy();
			frameData->projUBO.Destroy();
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

		ldrPipeline.Destroy();
		pbrPipeline.Destroy();

		ldrRenderPass.Destroy();
		hdrRenderPass.Destroy();

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
	AssetResources* Renderer::CreateAssetResources(AssetDisk* asset, PipelineType pipelineType)
	{
		assetResources.emplace_back(AssetResources());
		resourcesMap.insert({ asset->uuid, static_cast<uint32_t>(assetResources.size() - 1) });

		AssetResources& resources = assetResources.back();

		switch (pipelineType)
		{
		case PipelineType::PBR:
		{
			CreatePBRAssetResources(asset, resources);
			break;
		}
		case PipelineType::CUBEMAP_PREPROCESSING:
		case PipelineType::SKYBOX:
		{
			if (skyboxAssetUUID != INVALID_UUID)
			{
				LogError("Attempting to load skybox mesh more than once!");
				return nullptr;
			}

			CreateSkyboxAssetResources(asset, resources);
			break;
		}
		case PipelineType::FULLSCREEN_QUAD:
		{
			if (fullscreenQuadAssetUUID != INVALID_UUID)
			{
				LogError("Attempting to load fullscreen quad mesh more than once!");
				return nullptr;
			}

			CreateFullscreenQuadAssetResources(asset, resources);
			break;
		}
		default:
		{
			TNG_ASSERT_MSG(false, "Create asset resources for pipeline is not yet implemented!");
		}
		}

		CreateAssetCommandBuffer(&resources);

		return &resources;
	}

	void Renderer::CreatePBRAssetResources(AssetDisk* asset, AssetResources& out_resources)
	{
		uint64_t totalIndexCount = 0;
		uint32_t vBufferOffset = 0;

		//////////////////////////////
		//
		//	MESH
		//
		//////////////////////////////
		Mesh<PBRVertex>* currMesh = reinterpret_cast<Mesh<PBRVertex>*>(asset->mesh);

		// Create the vertex and index buffers
		uint64_t numVertexBytes = currMesh->vertices.size() * sizeof(PBRVertex);
		VertexBuffer& vb = out_resources.vertexBuffer;
		vb.Create(numVertexBytes);

		uint64_t numIndexBytes = currMesh->indices.size() * sizeof(IndexType);
		IndexBuffer& ib = out_resources.indexBuffer;
		ib.Create(numIndexBytes);

		{
			DisposableCommand command(QueueType::TRANSFER, true);
			vb.CopyIntoBuffer(command.GetBuffer(), currMesh->vertices.data(), numVertexBytes);
			ib.CopyIntoBuffer(command.GetBuffer(), currMesh->indices.data(), numIndexBytes);
		}

		// Destroy the staging buffers
		vb.DestroyIntermediateBuffers();
		ib.DestroyIntermediateBuffers();

		// Accumulate the index count of this mesh;
		totalIndexCount += currMesh->indices.size();

		// Set the current offset and then increment
		out_resources.offset = vBufferOffset++;

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
			asset->materials[0].SetName("Default Material");
		}
		Material& material = asset->materials[0];

		// Resize to the number of possible texture types
		out_resources.material.resize(static_cast<uint32_t>(Material::TEXTURE_TYPE::_COUNT));

		// Pre-emptively fill out the texture create info, so we can just pass it to all CreateFromFile() calls
		SamplerCreateInfo samplerInfo{};
		samplerInfo.minificationFilter = VK_FILTER_LINEAR;
		samplerInfo.magnificationFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.maxAnisotropy = 4.0;
		samplerInfo.enableAnisotropicFiltering = true;

		SamplerCreateInfo fallbackSamplerInfo{};
		samplerInfo.minificationFilter = VK_FILTER_NEAREST;
		samplerInfo.magnificationFilter = VK_FILTER_NEAREST;
		samplerInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.maxAnisotropy = 1.0;
		samplerInfo.enableAnisotropicFiltering = false;

		ImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.aspect = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.viewScope = ImageViewScope::ENTIRE_IMAGE;

		BaseImageCreateInfo baseImageInfo{};
		baseImageInfo.width = 0; // Unused
		baseImageInfo.height = 0; // Unused
		baseImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		baseImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		baseImageInfo.mipLevels = 1;
		baseImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		baseImageInfo.arrayLayers = 1;
		baseImageInfo.flags = 0;

		// Default material fallback
		BaseImageCreateInfo fallbackBaseImageInfo{};
		fallbackBaseImageInfo.width = 1;
		fallbackBaseImageInfo.height = 1;
		fallbackBaseImageInfo.mipLevels = 1;
		fallbackBaseImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		fallbackBaseImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		fallbackBaseImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		fallbackBaseImageInfo.arrayLayers = 1;
		fallbackBaseImageInfo.flags = 0;

		for (uint32_t i = 0; i < static_cast<uint32_t>(Material::TEXTURE_TYPE::_COUNT); i++)
		{
			Material::TEXTURE_TYPE texType = static_cast<Material::TEXTURE_TYPE>(i);

			// The only supported texture (currently) that stores actual colors is the diffuse map,
			// so we need to set it's format to sRGB instead of UNORM
			if (texType == Material::TEXTURE_TYPE::DIFFUSE)
			{
				baseImageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
				fallbackBaseImageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
			}
			else
			{
				baseImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
				fallbackBaseImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
			}

			if (material.HasTextureOfType(texType))
			{
				Texture* matTexture = material.GetTextureOfType(texType);
				TNG_ASSERT_MSG(matTexture != nullptr, "Why is this texture nullptr when we specifically checked against it?");

				TextureResource& texResource = out_resources.material[i];
				texResource.CreateFromFile(matTexture->fileName, &baseImageInfo, &viewCreateInfo, &samplerInfo);
			}
			else // use fallback
			{
				uint32_t data = DEFAULT_MATERIAL.at(texType);

				TextureResource& texResource = out_resources.material[i];
				texResource.Create(&fallbackBaseImageInfo, &viewCreateInfo, &fallbackSamplerInfo);
				texResource.CopyFromData(static_cast<void*>(&data), sizeof(data));
				texResource.TransitionLayout_Immediate(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}

		// Insert the asset's uuid into the assetDrawState map. We do not render it
		// upon insertion by default
		out_resources.shouldDraw = false;
		out_resources.transform = Transform();
		out_resources.indexCount = totalIndexCount;
		out_resources.uuid = asset->uuid;

		CreateAssetUniformBuffers(out_resources.uuid);
		CreateAssetDescriptorSets(out_resources.uuid);

		// Initialize the view + projection matrix UBOs to some values, so when new assets are created they get sensible defaults
		// for their descriptor sets. 
		// Note that we're operating under the assumption that assets will only be created before
		// we hit the update loop, simply because we're updating all frames in flight here. If this changes in the future, another
		// solution must be implemented
		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			InitializeDescriptorSets(out_resources.uuid, i);
		}
	}

	void Renderer::CreateSkyboxAssetResources(AssetDisk* asset, AssetResources& out_resources)
	{
		if (fullscreenQuadAssetUUID == INVALID_UUID)
		{
			LogError("Failed to load skybox. Fullscreen quad asset is not loaded when it's required to preprocess the skybox BRDF convolution map!");
			return;
		}

		LogInfo("Starting cubemap preprocessing...");

		uint64_t totalIndexCount = 0;
		uint32_t vBufferOffset = 0;

		Mesh<CubemapVertex>* currMesh = reinterpret_cast<Mesh<CubemapVertex>*>(asset->mesh);

		// Create the vertex buffer
		uint64_t numVertexBytes = currMesh->vertices.size() * sizeof(CubemapVertex);
		VertexBuffer& vb = out_resources.vertexBuffer;
		vb.Create(numVertexBytes);

		uint64_t numIndexBytes = currMesh->indices.size() * sizeof(IndexType);
		IndexBuffer& ib = out_resources.indexBuffer;
		ib.Create(numIndexBytes);
		{
			DisposableCommand command(QueueType::TRANSFER, true);
			vb.CopyIntoBuffer(command.GetBuffer(), currMesh->vertices.data(), numVertexBytes);
			ib.CopyIntoBuffer(command.GetBuffer(), currMesh->indices.data(), numIndexBytes);
		}

		// Destroy the staging buffers
		vb.DestroyIntermediateBuffers();
		ib.DestroyIntermediateBuffers();

		// Accumulate the index count of this mesh;
		totalIndexCount += currMesh->indices.size();

		// Set the current offset and then increment
		out_resources.offset = vBufferOffset++;

		out_resources.shouldDraw = false;
		out_resources.transform = Transform();
		out_resources.indexCount = totalIndexCount;
		out_resources.uuid = asset->uuid;

		cubemapPreprocessingPass.SetData(&descriptorPool, swapChainExtent);
		cubemapPreprocessingPass.Create();

		// Convert the HDR texture into a cubemap and calculate IBL components (irradiance + prefilter map + BRDF LUT)
		PrimaryCommandBuffer cmdBuffer;
		cmdBuffer.Create(GetCommandPool(QueueType::GRAPHICS));
		cmdBuffer.BeginRecording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);

		cubemapPreprocessingPass.Preprocess(&cmdBuffer, &out_resources, GetAssetResourcesFromUUID(fullscreenQuadAssetUUID));

		cmdBuffer.EndRecording();

		VkCommandBuffer commandBuffers[1] = { cmdBuffer.GetBuffer() };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = 0;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = commandBuffers;

		VkFence cubemapPreprocessingFence = cubemapPreprocessingPass.GetFence();
		vkResetFences(GetLogicalDevice(), 1, &cubemapPreprocessingFence);

		if (SubmitQueue(QueueType::GRAPHICS, &submitInfo, 1, cubemapPreprocessingFence) != VK_SUCCESS)
		{
			LogError("Failed to execute commands for cubemap preprocessing!");
			return;
		}

		// Wait for the GPU to finish preprocessing the cubemap
		vkWaitForFences(GetLogicalDevice(), 1, &cubemapPreprocessingFence, VK_TRUE, UINT64_MAX);

		cubemapPreprocessingPass.UpdatePrefilterMapViewScope();
		cubemapPreprocessingPass.DestroyIntermediates();

		LogInfo("Cubemap preprocessing done!");

		// Initialize the skybox pass
		skyboxPass.SetData(&descriptorPool, &hdrRenderPass, swapChainExtent);
		skyboxPass.Create();

		skyboxPass.UpdateSkyboxCubemapShaderParameter(cubemapPreprocessingPass.GetSkyboxCubemap());

		// Cache the skybox mesh UUID. We used it to convert the HDR equirectangular map to a cubemap, but we can
		// reuse the cube mesh to draw the skybox in future frames as well
		skyboxAssetUUID = asset->uuid;
	}

	void Renderer::CreateFullscreenQuadAssetResources(AssetDisk* asset, AssetResources& out_resources)
	{
		uint64_t totalIndexCount = 0;
		uint32_t vBufferOffset = 0;

		Mesh<UVVertex>* currMesh = reinterpret_cast<Mesh<UVVertex>*>(asset->mesh);

		// Create the vertex buffer
		uint64_t numVertexBytes = currMesh->vertices.size() * sizeof(UVVertex);
		VertexBuffer& vb = out_resources.vertexBuffer;
		vb.Create(numVertexBytes);

		uint64_t numIndexBytes = currMesh->indices.size() * sizeof(IndexType);
		IndexBuffer& ib = out_resources.indexBuffer;
		ib.Create(numIndexBytes);

		{
			DisposableCommand command(QueueType::TRANSFER, true);
			vb.CopyIntoBuffer(command.GetBuffer(), currMesh->vertices.data(), numVertexBytes);
			ib.CopyIntoBuffer(command.GetBuffer(), currMesh->indices.data(), numIndexBytes);
		}

		// Destroy the staging buffers
		vb.DestroyIntermediateBuffers();
		ib.DestroyIntermediateBuffers();

		// Accumulate the index count of this mesh;
		totalIndexCount += currMesh->indices.size();

		// Set the current offset and then increment
		out_resources.offset = vBufferOffset++;

		out_resources.shouldDraw = false;
		out_resources.transform = Transform();
		out_resources.indexCount = totalIndexCount;
		out_resources.uuid = asset->uuid;

		CreateLDRUniformBuffer();
		CreateLDRDescriptorSet();

		// Cache the UUID
		fullscreenQuadAssetUUID = asset->uuid;
	}

	void Renderer::CreateAssetCommandBuffer(AssetResources* resources)
	{
		UUID uuid = resources->uuid;

		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			auto* secondaryCmdBufferMap = &(GetFDDAtIndex(i)->assetCommandBuffers);
			// Ensure that there's not already an entry in the secondaryCommandBuffers map. We bail in case of a collision
			if (secondaryCmdBufferMap->find(uuid) != secondaryCmdBufferMap->end())
			{
				LogError("Attempted to create a secondary command buffer for an asset, but a secondary command buffer was already found for asset uuid %ull", uuid);
				return;
			}

			secondaryCmdBufferMap->emplace(uuid, SecondaryCommandBuffer());
			SecondaryCommandBuffer& commandBuffer = secondaryCmdBufferMap->at(uuid);
			commandBuffer.Create(GetCommandPool(QueueType::GRAPHICS));
		}
	}

	void Renderer::DestroyAssetBuffersHelper(AssetResources* resources)
	{
		// Destroy the vertex buffer
		resources->vertexBuffer.Destroy();

		// Destroy the index buffer
		resources->indexBuffer.Destroy();

		// Destroy textures
		for (auto& tex : resources->material)
		{
			tex.Destroy();
		}
	}

	VkFramebuffer Renderer::GetFramebufferAtIndex(uint32_t frameBufferIndex)
	{
		return GetSWIDDAtIndex(frameBufferIndex)->swapChainFramebuffer.GetFramebuffer();
	}

	void Renderer::DestroyAssetResources(UUID uuid)
	{
		AssetResources* asset = GetAssetResourcesFromUUID(uuid);
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
		
		// Remove the secondary command buffer
		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			auto frameData = GetFDDAtIndex(i);
			auto cmdBufferIter = frameData->assetCommandBuffers.find(uuid);
			if (cmdBufferIter != frameData->assetCommandBuffers.end())
			{
				frameData->assetCommandBuffers.erase(cmdBufferIter);
			}
		}
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
		auto frameData = GetCurrentFDD();
		auto& assetDescriptorMap = frameData->assetDescriptorDataMap;

		UpdateCameraDataUniformBuffers(currentFrame, position, viewMatrix);
		UpdateProjectionUniformBuffer(currentFrame);

		// Update the view matrix and camera position UBOs for all assets, as well as the descriptor sets unless they're not being drawn this frame
		for (auto& assetData : assetDescriptorMap)
		{
			UUID assetUUID = assetData.first;
			AssetResources* resources = GetAssetResourcesFromUUID(assetUUID);
			if (resources == nullptr)
			{
				continue;
			}

			// Don't update asset resources that are not being drawn this frame
			if (!resources->shouldDraw)
			{
				continue;
			}

			// Wait for the frame to finish using the camera buffer before updating it
			vkWaitForFences(GetLogicalDevice(), 1, &frameData->inFlightFence, VK_TRUE, UINT64_MAX);

			UpdateCameraDataDescriptorSet(assetUUID, currentFrame);
		}

		// Update the camera descriptor for the skybox as well
		skyboxPass.UpdateCameraMatricesShaderParameters(currentFrame, &frameData->viewUBO, &frameData->projUBO);
	}

	void Renderer::RecreateSwapChain()
	{
		VkDevice logicalDevice = GetLogicalDevice();

		vkDeviceWaitIdle(logicalDevice);

		CleanupSwapChain();

		CreateSwapChain();
		CreateColorAttachmentTextures();
		CreateDepthTextures();
		CreateFramebuffers();
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

		FrameDependentData* frameData = GetCurrentFDD();

		vkWaitForFences(logicalDevice, 1, &frameData->inFlightFence, VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		result = vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX,
			frameData->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

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
		vkResetFences(logicalDevice, 1, &(frameData->inFlightFence));

		PrimaryCommandBuffer* hdrCmdBuffer = &(frameData->hdrCommandBuffer);
		hdrCmdBuffer->Reset();
		hdrCmdBuffer->BeginRecording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
		hdrCmdBuffer->CMD_BeginRenderPass(&hdrRenderPass, &(frameData->hdrFramebuffer), swapChainExtent, true, true);

		// Record skybox commands
		DrawSkybox(hdrCmdBuffer);

		// Record PBR asset commands
		DrawAssets(hdrCmdBuffer);

		hdrCmdBuffer->CMD_EndRenderPass();
		hdrCmdBuffer->EndRecording();

		// Submit the LDR conversion commands separately, since they use a different render pass
		Framebuffer& swapChainFramebuffer = GetSWIDDAtIndex(imageIndex)->swapChainFramebuffer;
		PrimaryCommandBuffer* ldrCmdBuffer = &(frameData->ldrCommandBuffer);
		ldrCmdBuffer->Reset();
		ldrCmdBuffer->BeginRecording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
		ldrCmdBuffer->CMD_BeginRenderPass(&ldrRenderPass, &swapChainFramebuffer, swapChainExtent, false, true);

		PerformLDRConversion(ldrCmdBuffer);

		ldrCmdBuffer->CMD_EndRenderPass();
		ldrCmdBuffer->EndRecording();

		{
			std::array<VkCommandBuffer, 2> commandBuffers = { hdrCmdBuffer->GetBuffer(), ldrCmdBuffer->GetBuffer() };
			std::array<VkSemaphore, 1> signalSemaphores = { frameData->renderFinishedSemaphore };
			std::array<VkSemaphore, 1> waitSemaphores = { frameData->imageAvailableSemaphore };
			std::array<VkPipelineStageFlags, 1> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
			submitInfo.pWaitSemaphores = waitSemaphores.data();
			submitInfo.pWaitDstStageMask = waitStages.data();
			submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
			submitInfo.pCommandBuffers = commandBuffers.data();
			submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
			submitInfo.pSignalSemaphores = signalSemaphores.data();

			if (SubmitQueue(QueueType::GRAPHICS, &submitInfo, 1, frameData->inFlightFence) != VK_SUCCESS)
			{
				return;
			}
		}

		///////////////////////////////////////
		// 
		// Swap chain present
		//
		///////////////////////////////////////

		{
			std::array<VkSemaphore, 1> waitSemaphores = { frameData->renderFinishedSemaphore };

			VkPresentInfoKHR presentInfo{};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
			presentInfo.pWaitSemaphores = waitSemaphores.data();

			std::array<VkSwapchainKHR, 1> swapChains = { swapChain };
			presentInfo.swapchainCount = static_cast<uint32_t>(swapChains.size());
			presentInfo.pSwapchains = swapChains.data();
			presentInfo.pImageIndices = &imageIndex;
			presentInfo.pResults = nullptr;

			result = vkQueuePresentKHR(queues[QueueType::PRESENT], &presentInfo);
		}

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			RecreateSwapChain();
		}
		else if (result != VK_SUCCESS)
		{
			LogError("Failed to present swap chain image!");
		}
	}

	void Renderer::DrawSkybox(PrimaryCommandBuffer* cmdBuffer)
	{
		auto frameData = GetCurrentFDD();

		AssetResources* skyboxAsset = GetAssetResourcesFromUUID(skyboxAssetUUID);
		if (skyboxAsset == nullptr)
		{
			LogError("Skybox asset is not loaded! Failed to draw skybox");
			return;
		}

		SecondaryCommandBuffer* secondaryCmdBuffer = GetSecondaryCommandBufferFromUUID(skyboxAssetUUID);
		if (secondaryCmdBuffer == nullptr)
		{
			return;
		}

		DrawData data{};
		data.asset = skyboxAsset;
		data.cmdBuffer = secondaryCmdBuffer;
		data.framebuffer = &frameData->hdrFramebuffer;
		data.renderPass = &hdrRenderPass;
		data.framebufferWidth = framebufferWidth;
		data.framebufferHeight = framebufferHeight;

		skyboxPass.Draw(currentFrame, data);

		VkCommandBuffer vkCmdBuffer = secondaryCmdBuffer->GetBuffer();
		cmdBuffer->CMD_ExecuteSecondaryCommands(&vkCmdBuffer, 1);
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
		deviceFeatures.geometryShader = VK_TRUE;

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
		pbrPipeline.SetData(&hdrRenderPass, &pbrSetLayoutCache, swapChainExtent);
		pbrPipeline.Create();

		ldrPipeline.SetData(&ldrRenderPass, &ldrSetLayoutCache, swapChainExtent);
		ldrPipeline.Create();
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
		VkFormat depthAttachmentFormat = FindDepthFormat();

		hdrRenderPass.SetData(VK_FORMAT_R32G32B32A32_SFLOAT, depthAttachmentFormat);
		hdrRenderPass.Create();

		ldrRenderPass.SetData(VK_FORMAT_B8G8R8A8_SRGB);
		ldrRenderPass.Create();
	}

	void Renderer::CreateFramebuffers()
	{
		CreateLDRFramebuffers();
		CreateHDRFramebuffers();
	}

	void Renderer::CreateLDRFramebuffers()
	{
		auto& swidd = swapChainImageDependentData;

		for (size_t i = 0; i < GetSWIDDSize(); i++)
		{
			std::vector<TextureResource*> attachments =
			{
				&colorAttachment,
				&swidd[i].swapChainImage
			};

			std::vector<uint32_t> imageViewIndices =
			{
				0,
				0
			};

			FramebufferCreateInfo framebufferInfo{};
			framebufferInfo.renderPass = &ldrRenderPass;
			framebufferInfo.attachments = attachments;
			framebufferInfo.imageViewIndices = imageViewIndices;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			swidd[i].swapChainFramebuffer.Create(framebufferInfo);
		}
	}

	void Renderer::CreateHDRFramebuffers()
	{
		std::vector<TextureResource*> attachments =
		{
			&hdrAttachment,
			&hdrDepthBuffer
		};

		std::vector<uint32_t> imageViewIndices =
		{
			0,
			0
		};

		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			auto frameData = GetFDDAtIndex(i);

			FramebufferCreateInfo framebufferInfo{};
			framebufferInfo.renderPass = &hdrRenderPass;
			framebufferInfo.attachments = attachments;
			framebufferInfo.imageViewIndices = imageViewIndices;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			frameData->hdrFramebuffer.Create(framebufferInfo);
		}
	}

	void Renderer::CreatePrimaryCommandBuffers()
	{
		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			auto frameData = GetFDDAtIndex(i);

			frameData->hdrCommandBuffer.Create(GetCommandPool(QueueType::GRAPHICS));
			frameData->ldrCommandBuffer.Create(GetCommandPool(QueueType::GRAPHICS));
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
			auto frameData = GetFDDAtIndex(i);

			if(vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &(frameData->imageAvailableSemaphore)) != VK_SUCCESS)
			{
				TNG_ASSERT_MSG(false, "Failed to create image available semaphore!");
			}

			if(vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &(frameData->renderFinishedSemaphore)) != VK_SUCCESS)
			{
				TNG_ASSERT_MSG(false, "Failed to create render finished semaphore!");
			}

			if(vkCreateFence(logicalDevice, &fenceInfo, nullptr, &(frameData->inFlightFence)) != VK_SUCCESS)
			{
				TNG_ASSERT_MSG(false, "Failed to create in-flight fence!");
			}
		}
	}

	void Renderer::CreateAssetUniformBuffers(UUID uuid)
	{
		VkDeviceSize transformUBOSize = sizeof(TransformUBO);

		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			FrameDependentData* currentFDD = GetFDDAtIndex(i);
			AssetDescriptorData& assetDescriptorData = currentFDD->assetDescriptorDataMap[uuid];

			// Create the TransformUBO
			UniformBuffer& transUBO = assetDescriptorData.transformUBO;
			transUBO.Create(transformUBOSize);
			transUBO.MapMemory();
		}
	}

	void Renderer::CreateFrameUniformBuffers()
	{
		VkDeviceSize viewUBOSize = sizeof(ViewUBO);
		VkDeviceSize projUBOSize = sizeof(ProjUBO);
		VkDeviceSize cameraDataSize = sizeof(CameraDataUBO);

		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			auto frameData = GetFDDAtIndex(i);

			// Create the ViewUBO
			UniformBuffer& vpUBO = frameData->viewUBO;
			vpUBO.Create(viewUBOSize);
			vpUBO.MapMemory();

			// Create the ProjUBO
			UniformBuffer& projUBO = frameData->projUBO;
			projUBO.Create(projUBOSize);
			projUBO.MapMemory();

			// Create the camera data UBO
			UniformBuffer& cameraDataUBO = frameData->cameraDataUBO;
			cameraDataUBO.Create(cameraDataSize);
			cameraDataUBO.MapMemory();
		}
	}

	void Renderer::CreateLDRUniformBuffer()
	{
		VkDeviceSize ldrCameraDataUBOSize = sizeof(float);

		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			auto frameData = GetFDDAtIndex(i);

			frameData->ldrCameraDataUBO.Create(ldrCameraDataUBOSize);
			frameData->ldrCameraDataUBO.MapMemory();
		}
	}

	void Renderer::CreateAssetDescriptorSets(UUID uuid)
	{
		uint32_t fddSize = GetFDDSize();

		for (uint32_t i = 0; i < fddSize; i++)
		{
			FrameDependentData* currentFDD = GetFDDAtIndex(i);
			currentFDD->assetDescriptorDataMap.insert({ uuid, AssetDescriptorData() });
			AssetDescriptorData& assetDescriptorData = currentFDD->assetDescriptorDataMap[uuid];

			for (uint32_t j = 0; j < pbrSetLayoutCache.GetLayoutCount(); j++)
			{
				assetDescriptorData.descriptorSets.push_back(DescriptorSet());
				DescriptorSet* currentSet = &assetDescriptorData.descriptorSets.back();

				std::optional<DescriptorSetLayout> setLayoutOpt = pbrSetLayoutCache.GetSetLayout(j);
				if (!setLayoutOpt.has_value())
				{
					LogError("Failed to create asset descriptor set #%u for asset with UUID '%u'", j, uuid);
					continue;
				}
				currentSet->Create(descriptorPool, setLayoutOpt.value());
			}
		}
	}

	void Renderer::CreateLDRDescriptorSet()
	{
		if (ldrSetLayoutCache.GetLayoutCount() != 1)
		{
			LogError("Failed to create LDR descriptor set, invalid layout count! Expected (%u) vs. actual (%u)", 1, ldrSetLayoutCache.GetLayoutCount());
			return;
		}

		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			auto frameData = GetFDDAtIndex(i);

			frameData->ldrDescriptorSet.Create(descriptorPool, ldrSetLayoutCache.GetSetLayout(0).value());
		}
	}

	void Renderer::CreateDescriptorSetLayouts()
	{
		CreatePBRSetLayouts();
		CreateLDRSetLayouts();
	}

	void Renderer::CreatePBRSetLayouts()
	{
		// Holds PBR textures
		SetLayoutSummary persistentLayout(0);
		persistentLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // Diffuse texture
		persistentLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // Normal texture
		persistentLayout.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // Metallic texture
		persistentLayout.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // Roughness texture
		persistentLayout.AddBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // Lightmap texture
		persistentLayout.AddBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // Irradiance map (diffuse IBL)
		persistentLayout.AddBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // Prefilter map (specular IBL)
		persistentLayout.AddBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // BRDF convolution map (specular IBL)
		pbrSetLayoutCache.CreateSetLayout(persistentLayout, 0);

		// Holds ProjUBO
		SetLayoutSummary unstableLayout(1);
		unstableLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);           // Projection matrix
		unstableLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);         // Camera exposure
		pbrSetLayoutCache.CreateSetLayout(unstableLayout, 0);

		// Holds TransformUBO + ViewUBO + CameraDataUBO
		SetLayoutSummary volatileLayout(2);
		volatileLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);   // Transform matrix
		volatileLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT); // Camera data
		volatileLayout.AddBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);   // View matrix
		pbrSetLayoutCache.CreateSetLayout(volatileLayout, 0);
	}

	void Renderer::CreateLDRSetLayouts()
	{
		SetLayoutSummary layout(0);
		layout.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);	// HDR texture
		layout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);			// Camera exposure
		ldrSetLayoutCache.CreateSetLayout(layout, 0);
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

		const uint32_t numUniformBuffers = 8;
		const uint32_t numImageSamplers = 7;

		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = numUniformBuffers * GetFDDSize();
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = numImageSamplers * GetFDDSize();

		descriptorPool.Create(poolSizes.data(), static_cast<uint32_t>(poolSizes.size()), static_cast<uint32_t>(poolSizes.size()) * fddSize * CONFIG::MaxAssetCount, 0);
	}

	void Renderer::CreateDepthTextures()
	{
		VkFormat depthFormat = FindDepthFormat();

		// HDR depth buffer
		BaseImageCreateInfo imageInfo{};
		imageInfo.width = framebufferWidth;
		imageInfo.height = framebufferHeight;
		imageInfo.format = depthFormat;
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imageInfo.mipLevels = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.arrayLayers = 1;
		imageInfo.flags = 0;

		ImageViewCreateInfo imageViewInfo{};
		imageViewInfo.aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

		SamplerCreateInfo samplerInfo{};
		samplerInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.magnificationFilter = VK_FILTER_LINEAR;
		samplerInfo.minificationFilter = VK_FILTER_LINEAR;
		samplerInfo.maxAnisotropy = 1.0f;

		hdrDepthBuffer.Create(&imageInfo, &imageViewInfo, &samplerInfo);
	}

	void Renderer::CreateColorAttachmentTextures()
	{
		// Swap chain color attachment resolve
		BaseImageCreateInfo imageInfo{};
		imageInfo.width = swapChainExtent.width;
		imageInfo.height = swapChainExtent.height;
		imageInfo.format = swapChainImageFormat;
		imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.mipLevels = 1;
		imageInfo.samples = DeviceCache::Get().GetMaxMSAA();
		imageInfo.arrayLayers = 1;
		imageInfo.flags = 0;

		ImageViewCreateInfo imageViewInfo{};
		imageViewInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

		SamplerCreateInfo samplerInfo{};
		samplerInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.magnificationFilter = VK_FILTER_LINEAR;
		samplerInfo.minificationFilter = VK_FILTER_LINEAR;
		samplerInfo.maxAnisotropy = 1.0f;

		colorAttachment.Create(&imageInfo, &imageViewInfo, &samplerInfo);

		// HDR attachment
		imageInfo.width = swapChainExtent.width;
		imageInfo.height = swapChainExtent.height;
		imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.mipLevels = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.arrayLayers = 1;
		imageInfo.flags = 0;

		// Maybe used for post-processing? Not sure yet
		samplerInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.magnificationFilter = VK_FILTER_LINEAR;
		samplerInfo.minificationFilter = VK_FILTER_LINEAR;
		samplerInfo.maxAnisotropy = 1.0f;

		hdrAttachment.Create(&imageInfo, &imageViewInfo, &samplerInfo);
	}

	void Renderer::DrawAssets(PrimaryCommandBuffer* cmdBuffer)
	{
		std::vector<VkCommandBuffer> secondaryCmdBuffers;
		secondaryCmdBuffers.resize(assetResources.size()); // At most we can have the same number of cmd buffers as there are asset resources
		uint32_t secondaryCmdBufferCount = 0;
		for (auto& iter : assetResources)
		{
			UUID& uuid = iter.uuid;

			if (iter.shouldDraw)
			{
				SecondaryCommandBuffer* secondaryCmdBuffer = GetSecondaryCommandBufferFromUUID(uuid);
				if (secondaryCmdBuffer == nullptr)
				{
					continue;
				}

				UpdateTransformUniformBuffer(iter.transform, uuid);
				UpdateTransformDescriptorSet(uuid);

				RecordSecondaryCommandBuffer(secondaryCmdBuffer, &iter);

				secondaryCmdBuffers[secondaryCmdBufferCount++] = secondaryCmdBuffer->GetBuffer();
			}
		}

		// Don't attempt to execute 0 command buffers
		if (secondaryCmdBufferCount > 0)
		{
			cmdBuffer->CMD_ExecuteSecondaryCommands(secondaryCmdBuffers.data(), secondaryCmdBufferCount);
		}
	}

	void Renderer::RecordSecondaryCommandBuffer(SecondaryCommandBuffer* cmdBuffer, AssetResources* resources)
	{
		auto frameData = GetCurrentFDD();

		// Retrieve the vector of descriptor sets for the given asset
		auto& descSets = frameData->assetDescriptorDataMap[resources->uuid].descriptorSets;
		std::vector<VkDescriptorSet> vkDescSets(descSets.size());
		for (uint32_t i = 0; i < descSets.size(); i++)
		{
			vkDescSets[i] = descSets[i].GetDescriptorSet();
		}

		VkCommandBufferInheritanceInfo inheritanceInfo{};
		inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.pNext = nullptr;
		inheritanceInfo.renderPass = hdrRenderPass.GetRenderPass();
		inheritanceInfo.subpass = 0;
		inheritanceInfo.framebuffer = frameData->hdrFramebuffer.GetFramebuffer();

		cmdBuffer->BeginRecording(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, &inheritanceInfo);

		cmdBuffer->CMD_BindMesh(resources);
		cmdBuffer->CMD_BindDescriptorSets(&pbrPipeline, static_cast<uint32_t>(vkDescSets.size()), vkDescSets.data());
		cmdBuffer->CMD_BindGraphicsPipeline(&pbrPipeline);
		cmdBuffer->CMD_SetScissor({ 0, 0 }, swapChainExtent);
		cmdBuffer->CMD_SetViewport(static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height));
		cmdBuffer->CMD_DrawIndexed(resources->indexCount);

		cmdBuffer->EndRecording();
	}

	void Renderer::PerformLDRConversion(PrimaryCommandBuffer* cmdBuffer)
	{
		UpdateLDRUniformBuffer();
		UpdateLDRDescriptorSet();

		AssetResources* fullscreenQuadAsset = GetAssetResourcesFromUUID(fullscreenQuadAssetUUID);
		auto frameData = GetCurrentFDD();

		cmdBuffer->CMD_BindGraphicsPipeline(&ldrPipeline);
		cmdBuffer->CMD_BindDescriptorSets(&ldrPipeline, 1, reinterpret_cast<VkDescriptorSet*>(&frameData->ldrDescriptorSet));
		cmdBuffer->CMD_SetScissor({ 0, 0 }, swapChainExtent);
		cmdBuffer->CMD_SetViewport(static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height));
		cmdBuffer->CMD_BindMesh(fullscreenQuadAsset);
		cmdBuffer->CMD_DrawIndexed(fullscreenQuadAsset->indexCount);

		// NOTE - color attachment is cleared at the beginning of the frame, so transitioning the layout to something
		//        else won't make a difference
	}

	void Renderer::RecreateAllSecondaryCommandBuffers()
	{
		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			auto frameData = GetFDDAtIndex(i);
			for (auto& resources : assetResources)
			{
				auto iter = frameData->assetCommandBuffers.find(resources.uuid);
				if (iter != frameData->assetCommandBuffers.end())
				{
					SecondaryCommandBuffer* commandBuffer = &iter->second;
					commandBuffer->Create(GetCommandPool(QueueType::GRAPHICS));
				}
			}
		}
	}

	void Renderer::CleanupSwapChain()
	{
		VkDevice logicalDevice = GetLogicalDevice();

		hdrAttachment.Destroy();
		colorAttachment.Destroy();

		hdrDepthBuffer.Destroy();

		for (auto& swidd : swapChainImageDependentData)
		{
			swidd.swapChainFramebuffer.Destroy();
		}

		for (auto& swidd : swapChainImageDependentData)
		{
			swidd.swapChainImage.DestroyImageViews();
		}

		vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
	}

	void Renderer::UpdateProjectionUniformBuffer(uint32_t frameIndex)
	{
		using namespace glm;

		// We assume that ProjUBO only has a projection matrix. If that changes we need to change the code below too
		TNG_ASSERT_COMPILE(sizeof(ProjUBO) == 64);

		ProjUBO projUBO;
		projUBO.proj = startingProjectionMatrix;

		auto frameData = GetFDDAtIndex(frameIndex);
		frameData->projUBO.UpdateData(&projUBO, sizeof(ProjUBO));
	}

	void Renderer::UpdateProjectionDescriptorSet(UUID uuid, uint32_t frameIndex)
	{
		auto frameData = GetFDDAtIndex(frameIndex);
		auto& currentAssetDataMap = frameData->assetDescriptorDataMap[uuid];

		DescriptorSet& descSet = currentAssetDataMap.descriptorSets[1];

		// Update ProjUBO descriptor set
		WriteDescriptorSets writeDescSets(1, 0);
		writeDescSets.AddUniformBuffer(descSet.GetDescriptorSet(), 0, &frameData->projUBO);
		descSet.Update(writeDescSets);
	}

	void Renderer::UpdatePBRTextureDescriptorSet(UUID uuid, uint32_t frameIndex)
	{
		FrameDependentData* currentFDD = GetFDDAtIndex(frameIndex);
		auto& currentAssetDataMap = currentFDD->assetDescriptorDataMap[uuid];

		DescriptorSet& descSet = currentAssetDataMap.descriptorSets[0];

		// Get the asset resources so we can retrieve the textures
		AssetResources* asset = GetAssetResourcesFromUUID(uuid);
		if (asset == nullptr)
		{
			return;
		}

		// Update PBR textures
		WriteDescriptorSets writeDescSets(0, 8);
		writeDescSets.AddImageSampler(descSet.GetDescriptorSet(), 0, &asset->material[static_cast<uint32_t>(Material::TEXTURE_TYPE::DIFFUSE)]);
		writeDescSets.AddImageSampler(descSet.GetDescriptorSet(), 1, &asset->material[static_cast<uint32_t>(Material::TEXTURE_TYPE::NORMAL)]);
		writeDescSets.AddImageSampler(descSet.GetDescriptorSet(), 2, &asset->material[static_cast<uint32_t>(Material::TEXTURE_TYPE::METALLIC)]);
		writeDescSets.AddImageSampler(descSet.GetDescriptorSet(), 3, &asset->material[static_cast<uint32_t>(Material::TEXTURE_TYPE::ROUGHNESS)]);
		writeDescSets.AddImageSampler(descSet.GetDescriptorSet(), 4, &asset->material[static_cast<uint32_t>(Material::TEXTURE_TYPE::LIGHTMAP)]);
		writeDescSets.AddImageSampler(descSet.GetDescriptorSet(), 5, cubemapPreprocessingPass.GetIrradianceMap());
		writeDescSets.AddImageSampler(descSet.GetDescriptorSet(), 6, cubemapPreprocessingPass.GetPrefilterMap());
		writeDescSets.AddImageSampler(descSet.GetDescriptorSet(), 7, cubemapPreprocessingPass.GetBRDFConvolutionMap());

		descSet.Update(writeDescSets);
	}

	void Renderer::UpdateLDRDescriptorSet()
	{
		FrameDependentData* frameData = GetCurrentFDD();
		DescriptorSet& descSet = frameData->ldrDescriptorSet;

		// Update view matrix + camera data descriptor set
		WriteDescriptorSets writeDescSets(1, 1);
		writeDescSets.AddImageSampler(descSet.GetDescriptorSet(), 0, &hdrAttachment);
		writeDescSets.AddUniformBuffer(descSet.GetDescriptorSet(), 1, &frameData->ldrCameraDataUBO);
		descSet.Update(writeDescSets);
	}

	void Renderer::UpdateCameraDataDescriptorSet(UUID uuid, uint32_t frameIndex)
	{
		auto frameData = GetFDDAtIndex(frameIndex);
		auto& currentAssetDataMap = frameData->assetDescriptorDataMap[uuid];

		DescriptorSet& descSet = currentAssetDataMap.descriptorSets[2];

		// Update view matrix + camera data descriptor set
		WriteDescriptorSets writeDescSets(2, 0);
		writeDescSets.AddUniformBuffer(descSet.GetDescriptorSet(), 2, &frameData->viewUBO);
		writeDescSets.AddUniformBuffer(descSet.GetDescriptorSet(), 0, &frameData->cameraDataUBO);
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

	void Renderer::UpdateCameraDataUniformBuffers(uint32_t frameIndex, const glm::vec3& position, const glm::mat4& viewMatrix)
	{
		auto frameData = GetFDDAtIndex(frameIndex);

		ViewUBO viewUBO{};
		viewUBO.view = viewMatrix;
		frameData->viewUBO.UpdateData(&viewUBO, sizeof(ViewUBO));

		CameraDataUBO cameraDataUBO{};
		cameraDataUBO.position = glm::vec4(position, 1.0f);
		cameraDataUBO.exposure = 1.0f;
		frameData->cameraDataUBO.UpdateData(&cameraDataUBO, sizeof(CameraDataUBO));
	}

	void Renderer::UpdateTransformDescriptorSet(UUID uuid)
	{
		auto frameData = GetCurrentFDD();
		auto& currentAssetDataMap = frameData->assetDescriptorDataMap[uuid];

		DescriptorSet& descSet = currentAssetDataMap.descriptorSets[2];

		// Update transform + cameraData descriptor sets
		WriteDescriptorSets writeDescSets(2, 0);
		writeDescSets.AddUniformBuffer(descSet.GetDescriptorSet(), 0, &currentAssetDataMap.transformUBO);
		writeDescSets.AddUniformBuffer(descSet.GetDescriptorSet(), 1, &frameData->cameraDataUBO);
		descSet.Update(writeDescSets);
	}

	void Renderer::InitializeDescriptorSets(UUID uuid, uint32_t frameIndex)
	{
		// Update all descriptor sets
		UpdateCameraDataDescriptorSet(uuid, frameIndex);
		UpdateProjectionDescriptorSet(uuid, frameIndex);
		UpdatePBRTextureDescriptorSet(uuid, frameIndex);
	}

	void Renderer::InitializeFrameUniformBuffers()
	{
		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			UpdateCameraDataUniformBuffers(i, startingCameraPosition, startingCameraViewMatrix);
			UpdateProjectionUniformBuffer(i);
		}
	}

	void Renderer::UpdateLDRUniformBuffer()
	{
		float exposure = 1.0f;

		auto frameData = GetCurrentFDD();
		frameData->ldrCameraDataUBO.UpdateData(&exposure, sizeof(exposure));
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

	SecondaryCommandBuffer* Renderer::GetSecondaryCommandBufferFromUUID(UUID uuid)
	{
		auto frameData = GetCurrentFDD();

		auto secondaryCmdBufferIter = frameData->assetCommandBuffers.find(uuid);
		if (secondaryCmdBufferIter == frameData->assetCommandBuffers.end())
		{
			LogWarning("Asset with UUID '%ull' doesn't have a secondary command buffer?", uuid);
			// Should we create a secondary command buffer here?

			return nullptr;
		}

		return &(secondaryCmdBufferIter->second);
	}

	void Renderer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		DisposableCommand command(QueueType::TRANSFER, true);

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