
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
		swapChainImageFormat(VK_FORMAT_UNDEFINED), swapChainExtent({ 0, 0 }), m_swapChainData(),
		currentFrame(0), descriptorPool(), framebufferWidth(0), framebufferHeight(0)
	{ }

	void Renderer::Initialize(GLFWwindow* windowHandle, uint32_t windowWidth, uint32_t windowHeight)
	{
		framebufferWidth = windowWidth;
		framebufferHeight = windowHeight;

		// Initialize Vulkan-related objects
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface(windowHandle);
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateDescriptorPool();
		CreateRenderPasses();
		CreateCommandPools();
		CreateColorAttachmentTextures();
		CreateFramebuffers();
		CreateSyncObjects();

		// Setup all the passes
		//cubemapPreprocessingPass.LoadTextureResources();
		//bloomPass.Create(swapChainExtent.width, swapChainExtent.height);
	}

	void Renderer::Update(float deltaTime)
	{
		UNUSED(deltaTime);

		if (swapChainExtent.width != framebufferWidth || swapChainExtent.height != framebufferHeight)
		{
			RecreateSwapChain();
		}
	}

	void Renderer::BeginFrame()
	{
		VkDevice logicalDevice = GetLogicalDevice();
		FrameData* frameData = GetCurrentFrameData();
		VkResult result = VK_SUCCESS;

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
			LogError("Failed to acquire swap chain image! Vulkan result: %u", static_cast<uint32_t>(result));
		}

		// Only reset the fence if we're submitting work, otherwise we might deadlock
		vkResetFences(logicalDevice, 1, &(frameData->inFlightFence));

		// Reset all previous data
		for (auto& primaryCmdBuffer : frameData->primaryCmdBuffers)
		{
			primaryCmdBuffer.Destroy();
		}
		frameData->primaryCmdBuffers.clear();

		for (auto& secondaryCmdBuffer : frameData->secondaryCmdBuffers)
		{
			secondaryCmdBuffer.Destroy();
		}
		frameData->secondaryCmdBuffers.clear();

		// Cache the swap chain image index we'll render to
		frameData->swapChainImageIndex = imageIndex;
	}

	void Renderer::EndFrame()
	{
		FrameData* frameData = GetCurrentFrameData();
		VkResult result = VK_SUCCESS;

		// Swap the back buffer to present
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &(frameData->renderFinishedSemaphore);
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain;
		presentInfo.pImageIndices = &(frameData->swapChainImageIndex);
		presentInfo.pResults = nullptr;
		result = vkQueuePresentKHR(queues[QUEUE_TYPE::PRESENT], &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			RecreateSwapChain();
		}
		else if (result != VK_SUCCESS)
		{
			LogError("Failed to present swap chain image!");
		}

		// Finally, incremement the currentFrame index
		// TODO - Replace this with a RenderContext and do this in EndFrame() instead
		currentFrame = (currentFrame + 1) % CONFIG::MaxFramesInFlight;
	}

	void Renderer::Draw()
	{
		//DrawFrame();

		// Clear the asset draw states after drawing the current frame
		// TODO - This is pretty slow to do per-frame, so I need to find a better way to
		//        clear the asset draw states. Maybe a sorted pool would work better but
		//        I want to avoid premature optimization so this'll do for now
		//for (auto& resources : assetResources)
		//{
		//	resources.shouldDraw = false;
		//}

		// Submit all command queues containing valid command buffers (and their respective submit infos)
		while(m_cmdQueuesToSubmit.size() != 0)
		{
			auto& [cmdBuffer, submitInfo] = m_cmdQueuesToSubmit.front();
			TNG_ASSERT_MSG(cmdBuffer.IsValid(), "Attempting to queue an invalid command buffer?");

			if (cmdBuffer.IsValid())
			{
				QUEUE_TYPE queueType = cmdBuffer.GetAllocatedQueueType();
				VkCommandBuffer vkCmdBuffer = cmdBuffer.GetBuffer();

				// Build submit struct and submit the queue type
				VkSubmitInfo vkSubmitInfo{};
				vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				vkSubmitInfo.waitSemaphoreCount = (submitInfo.waitSemaphore != VK_NULL_HANDLE ? 1 : 0);
				vkSubmitInfo.pWaitSemaphores = &submitInfo.waitSemaphore;
				vkSubmitInfo.pWaitDstStageMask = &submitInfo.waitStages;
				vkSubmitInfo.commandBufferCount = 1;
				vkSubmitInfo.pCommandBuffers = &vkCmdBuffer;
				vkSubmitInfo.signalSemaphoreCount = (submitInfo.signalSemaphore != VK_NULL_HANDLE ? 1 : 0);
				vkSubmitInfo.pSignalSemaphores = &submitInfo.signalSemaphore;

				VkResult result = SubmitQueue(queueType, &vkSubmitInfo, 1, submitInfo.fence);
				if (result != VK_SUCCESS)
				{
					LogError("Failed to submit command queue! Queue type: %u", static_cast<uint32_t>(queueType));
					continue;
				}

				// Proceed to the next command queue
				m_cmdQueuesToSubmit.pop();
			}
		}

	}

	void Renderer::Shutdown()
	{
		VkDevice logicalDevice = DeviceCache::Get().GetLogicalDevice();

		vkDeviceWaitIdle(logicalDevice);

		//DestroyAllAssetResources();

		if (m_rendererShutdownCallback)
		{
			m_rendererShutdownCallback();
		}

		CleanupSwapChain();

		//ldrPass.Destroy();
		//pbrPass.Destroy();
		//cubemapPreprocessingPass.Destroy();
		//skyboxPass.Destroy();
		//bloomPass.Destroy();

		//for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
		//{
		//	auto frameData = GetFrameData(i);

		//	frameData->hdrFramebuffer.Destroy();
		//}

		descriptorPool.Destroy();

		for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
		{
			auto frameData = GetFrameData(i);
			vkDestroySemaphore(logicalDevice, frameData->imageAvailableSemaphore, nullptr);
			vkDestroySemaphore(logicalDevice, frameData->renderFinishedSemaphore, nullptr);
			vkDestroyFence(logicalDevice, frameData->inFlightFence, nullptr);
		}

		CommandPoolRegistry::Get().DestroyPools();

		renderPass.Destroy();
		//hdrRenderPass.Destroy();

		vkDestroyDevice(logicalDevice, nullptr);
		DeviceCache::Get().InvalidateCache();

		vkDestroySurfaceKHR(vkInstance, surface, nullptr);

		if (enableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, nullptr);
		}

		vkDestroyInstance(vkInstance, nullptr);
	}

	DescriptorSet Renderer::AllocateDescriptorSet(const DescriptorSetLayout& setLayout)
	{
		DescriptorSet s;
		s.Create(descriptorPool, setLayout);

		FrameData* fd = GetCurrentFrameData();
		fd->descriptorSets.push_back(s);

		return s;
	}

	PrimaryCommandBuffer Renderer::AllocatePrimaryCommandBuffer(QUEUE_TYPE type)
	{
		PrimaryCommandBuffer cb;
		cb.Allocate(type);

		FrameData* fd = GetCurrentFrameData();
		fd->primaryCmdBuffers.push_back(cb);

		return cb;
	}

	SecondaryCommandBuffer Renderer::AllocateSecondaryCommandBuffer(QUEUE_TYPE type)
	{
		SecondaryCommandBuffer cb;
		cb.Allocate(type);

		FrameData* fd = GetCurrentFrameData();
		fd->secondaryCmdBuffers.push_back(cb);

		return cb;
	}

	bool Renderer::CreateSemaphore(VkSemaphore* semaphore, const VkSemaphoreCreateInfo& createInfo)
	{
		if (semaphore == nullptr)
		{
			LogError("Failed to create semaphore. Semaphore pointer is null!");
			return false;
		}
		return (vkCreateSemaphore(GetLogicalDevice(), &createInfo, nullptr, semaphore) == VK_SUCCESS);
	}

	void Renderer::DestroySemaphore(VkSemaphore* semaphore)
	{
		if (semaphore != nullptr)
		{
			vkDestroySemaphore(GetLogicalDevice(), *semaphore, nullptr);
		}
	}

	bool Renderer::CreateFence(VkFence* fence, const VkFenceCreateInfo& createInfo)
	{
		if (fence == nullptr)
		{
			LogError("Failed to create fence. Fence pointer is null!");
			return false;
		}
		return (vkCreateFence(GetLogicalDevice(), &createInfo, nullptr, fence) == VK_SUCCESS);
	}

	void Renderer::DestroyFence(VkFence* fence)
	{
		if (fence != nullptr)
		{
			vkDestroyFence(GetLogicalDevice(), *fence, nullptr);
		}
	}

	void Renderer::QueueCommandBuffer(const PrimaryCommandBuffer& cmd, const QueueSubmitInfo& info)
	{
		if (cmd.IsValid())
		{
			m_cmdQueuesToSubmit.push(std::make_pair<>(cmd, info));
		}
	}

	//void Renderer::CreateAssetCommandBuffer(AssetResources* resources)
	//{
	//	UUID uuid = resources->uuid;

	//	for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight(); i++)
	//	{
	//		auto* secondaryCmdBufferMap = &(GetFDDAtIndex(i)->assetCommandBuffers);
	//		// Ensure that there's not already an entry in the secondaryCommandBuffers map. We bail in case of a collision
	//		if (secondaryCmdBufferMap->find(uuid) != secondaryCmdBufferMap->end())
	//		{
	//			LogError("Attempted to create a secondary command buffer for an asset, but a secondary command buffer was already found for asset uuid %ull", uuid);
	//			return;
	//		}

	//		secondaryCmdBufferMap->emplace(uuid, SecondaryCommandBuffer());
	//		SecondaryCommandBuffer& commandBuffer = secondaryCmdBufferMap->at(uuid);
	//		commandBuffer.Allocate(QUEUE_TYPE::GRAPHICS);
	//	}
	//}

	VkFramebuffer Renderer::GetFramebufferAtIndex(uint32_t frameBufferIndex)
	{
		return GetSWIDDAtIndex(frameBufferIndex)->swapChainFramebuffer.GetFramebuffer();
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

	Framebuffer* Renderer::GetCurrentSwapChainFramebuffer()
	{
		uint32_t imageIndex = m_frameData[currentFrame].swapChainImageIndex;
		return &m_swapChainData[imageIndex].swapChainFramebuffer;
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

	void Renderer::WaitForFence(VkFence fence, uint64_t timeout)
	{
		LogInfo("Waiting for fence with timeout %u", timeout);
		vkWaitForFences(GetLogicalDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
	}

	uint32_t Renderer::GetCurrentFrameIndex() const
	{
		return currentFrame;
	}

	VkSemaphore Renderer::GetCurrentImageAvailableSemaphore() const
	{
		return m_frameData[currentFrame].imageAvailableSemaphore;
	}

	VkSemaphore Renderer::GetCurrentRenderFinishedSemaphore() const
	{
		return m_frameData[currentFrame].renderFinishedSemaphore;
	}

	VkFence Renderer::GetCurrentFrameFence() const
	{
		return m_frameData[currentFrame].inFlightFence;
	}

	void Renderer::RegisterSwapChainRecreatedCallback(SwapChainRecreatedCallback callback)
	{
		m_swapChainRecreatedCallback = callback;
	}

	void Renderer::RegisterRendererShutdownCallback(RendererShutdownCallback callback)
	{
		m_rendererShutdownCallback = callback;
	}

	void Renderer::RecreateSwapChain()
	{
		VkDevice logicalDevice = GetLogicalDevice();

		vkDeviceWaitIdle(logicalDevice);

		CleanupSwapChain();

		CreateSwapChain();
		CreateColorAttachmentTextures();
		CreateFramebuffers();

		// Let the application know that the swap chain was resized
		if (m_swapChainRecreatedCallback != nullptr)
		{
			m_swapChainRecreatedCallback(framebufferWidth, framebufferHeight);
		}
	}

	//void Renderer::SetAssetDrawState(UUID uuid)
	//{
	//	AssetResources* resources = GetAssetResourcesFromUUID(uuid);
	//	if (resources == nullptr)
	//	{
	//		// Maybe the asset resources were deleted but we somehow forgot to remove it from the assetDrawStates map?
	//		LogError(false, "Attempted to set asset draw state, but asset does not exist or UUID is invalid!");
	//		return;
	//	}

	//	resources->shouldDraw = true;
	//}

	//void Renderer::SetAssetTransform(UUID uuid, const Transform& transform)
	//{
	//	AssetResources* asset = GetAssetResourcesFromUUID(uuid);
	//	if (asset == nullptr)
	//	{
	//		return;
	//	}

	//	asset->transform = transform;
	//}

	//void Renderer::SetAssetPosition(UUID uuid, const glm::vec3& position)
	//{
	//	AssetResources* asset = GetAssetResourcesFromUUID(uuid);
	//	if (asset == nullptr)
	//	{
	//		return;
	//	}

	//	Transform& transform = asset->transform;
	//	transform.position = position;
	//}

	//void Renderer::SetAssetRotation(UUID uuid, const glm::vec3& rotation)
	//{
	//	AssetResources* asset = GetAssetResourcesFromUUID(uuid);
	//	if (asset == nullptr)
	//	{
	//		return;
	//	}

	//	Transform& transform = asset->transform;
	//	transform.rotation = rotation;
	//}

	//void Renderer::SetAssetScale(UUID uuid, const glm::vec3& scale)
	//{
	//	AssetResources* asset = GetAssetResourcesFromUUID(uuid);
	//	if (asset == nullptr)
	//	{
	//		return;
	//	}

	//	Transform& transform = asset->transform;
	//	transform.scale = scale;
	//}

	//void Renderer::DrawFrame()
	//{
	//	VkDevice logicalDevice = GetLogicalDevice();
	//	VkResult result = VK_SUCCESS;

	//	FrameDependentData* frameData = GetCurrentFDD();

	//	vkWaitForFences(logicalDevice, 1, &frameData->inFlightFence, VK_TRUE, UINT64_MAX);

	//	uint32_t imageIndex;
	//	result = vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX,
	//		frameData->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	//	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	//	{
	//		RecreateSwapChain();
	//		return;
	//	}
	//	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	//	{
	//		LogError("Failed to acquire swap chain image! Vulkan result: %u", static_cast<uint32_t>(result));
	//	}

	//	// Only reset the fence if we're submitting work, otherwise we might deadlock
	//	vkResetFences(logicalDevice, 1, &(frameData->inFlightFence));

	//	// Delete commands from previous frame index
	//	//frameData->hdrCommandBuffer.Destroy();
	//	//frameData->ldrCommandBuffer.Destroy();
	//	//frameData->postProcessingCommandBuffer.Destroy();

	//	///////////////////////////////////////
	//	// 
	//	// CORE RENDERING
	//	//
	//	///////////////////////////////////////

	//	//auto& hdrCmdBuffer = frameData->hdrCommandBuffer;
	//	//hdrCmdBuffer.Allocate(QUEUE_TYPE::GRAPHICS);
	//	//hdrCmdBuffer.BeginRecording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
	//	//hdrCmdBuffer.CMD_BeginRenderPass(&hdrRenderPass, &(frameData->hdrFramebuffer), swapChainExtent, true, true);

	//	//// Record skybox commands
	//	//DrawSkybox(&hdrCmdBuffer);

	//	//// Record PBR asset commands
	//	//DrawAssets(&hdrCmdBuffer);

	//	//hdrCmdBuffer.CMD_EndRenderPass();
	//	//hdrCmdBuffer.EndRecording();

	//	///////////////////////////////////////
	//	// 
	//	// POST-PROCESSING
	//	//
	//	///////////////////////////////////////

	//	//auto& postProcessingCmdBuffer = frameData->postProcessingCommandBuffer;
	//	//postProcessingCmdBuffer.Allocate(QUEUE_TYPE::COMPUTE);
	//	//postProcessingCmdBuffer.BeginRecording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
	//	//
	//	//bloomPass.Draw(currentFrame, &postProcessingCmdBuffer, &frameData->hdrAttachment);

	//	//postProcessingCmdBuffer.EndRecording();

	//	//// Submit the LDR conversion commands separately, since they use a different render pass
	//	//Framebuffer& swapChainFramebuffer = GetSWIDDAtIndex(imageIndex)->swapChainFramebuffer;
	//	//auto& ldrCmdBuffer = frameData->ldrCommandBuffer;
	//	//ldrCmdBuffer.Allocate(QUEUE_TYPE::GRAPHICS);
	//	//ldrCmdBuffer.BeginRecording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
	//	//ldrCmdBuffer.CMD_BeginRenderPass(&ldrRenderPass, &swapChainFramebuffer, swapChainExtent, false, true);

	//	//PerformLDRConversion(&ldrCmdBuffer);

	//	//ldrCmdBuffer.CMD_EndRenderPass();
	//	//ldrCmdBuffer.EndRecording();

	//	///////////////////////////////////////
	//	// 
	//	// QUEUE SUBMISSION
	//	//
	//	///////////////////////////////////////

	//	//result = SubmitCoreRenderingQueue(&hdrCmdBuffer, frameData);
	//	//result = SubmitPostProcessingQueue(&postProcessingCmdBuffer, frameData);
	//	//result = SubmitLDRConversionQueue(&ldrCmdBuffer, frameData);

	//	///////////////////////////////////////
	//	// 
	//	// SWAP CHAIN PRESENT
	//	//
	//	///////////////////////////////////////

	//	{
	//		std::array<VkSemaphore, 1> waitSemaphores = { frameData->renderFinishedSemaphore };

	//		VkPresentInfoKHR presentInfo{};
	//		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	//		presentInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
	//		presentInfo.pWaitSemaphores = waitSemaphores.data();

	//		std::array<VkSwapchainKHR, 1> swapChains = { swapChain };
	//		presentInfo.swapchainCount = static_cast<uint32_t>(swapChains.size());
	//		presentInfo.pSwapchains = swapChains.data();
	//		presentInfo.pImageIndices = &imageIndex;
	//		presentInfo.pResults = nullptr;

	//		result = vkQueuePresentKHR(queues[QUEUE_TYPE::PRESENT], &presentInfo);
	//	}

	//	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	//	{
	//		RecreateSwapChain();
	//	}
	//	else if (result != VK_SUCCESS)
	//	{
	//		LogError("Failed to present swap chain image!");
	//	}
	//}

	//void Renderer::DrawAssets(PrimaryCommandBuffer* cmdBuffer)
	//{
	//	std::vector<VkCommandBuffer> secondaryCmdBuffers;
	//	secondaryCmdBuffers.resize(assetResources.size()); // At most we can have the same number of cmd buffers as there are asset resources
	//	uint32_t secondaryCmdBufferCount = 0;

	//	for (AssetResources& resources : assetResources)
	//	{
	//		UUID& uuid = resources.uuid;

	//		if (resources.shouldDraw)
	//		{
	//			SecondaryCommandBuffer* secondaryCmdBuffer = GetSecondaryCommandBufferFromUUID(uuid);
	//			if (secondaryCmdBuffer == nullptr)
	//			{
	//				continue;
	//			}

	//			// Update the transform for the asset
	//			pbrPass.UpdateTransformUniformBuffer(currentFrame, resources.transform);

	//			// TODO - This is super hard-coded, but it'll do for now
	//			std::array<const TextureResource*, 8> textures;
	//			textures[0] = &resources.material[static_cast<uint32_t>(Material::TEXTURE_TYPE::DIFFUSE)];
	//			textures[1] = &resources.material[static_cast<uint32_t>(Material::TEXTURE_TYPE::NORMAL)];
	//			textures[2] = &resources.material[static_cast<uint32_t>(Material::TEXTURE_TYPE::METALLIC)];
	//			textures[3] = &resources.material[static_cast<uint32_t>(Material::TEXTURE_TYPE::ROUGHNESS)];
	//			textures[4] = &resources.material[static_cast<uint32_t>(Material::TEXTURE_TYPE::LIGHTMAP)];
	//			textures[5] = cubemapPreprocessingPass.GetIrradianceMap();
	//			textures[6] = cubemapPreprocessingPass.GetPrefilterMap();
	//			textures[7] = cubemapPreprocessingPass.GetBRDFConvolutionMap();
	//			pbrPass.UpdateDescriptorSets(currentFrame, textures);

	//			DrawData drawData{};
	//			drawData.asset = &resources;
	//			drawData.cmdBuffer = secondaryCmdBuffer;
	//			drawData.framebuffer = &(GetCurrentFDD()->hdrFramebuffer);
	//			drawData.renderPass = &hdrRenderPass;
	//			drawData.framebufferWidth = framebufferWidth;
	//			drawData.framebufferHeight = framebufferHeight;
	//			pbrPass.Draw(currentFrame, drawData);

	//			secondaryCmdBuffers[secondaryCmdBufferCount++] = secondaryCmdBuffer->GetBuffer();
	//		}
	//	}

	//	// Don't attempt to execute 0 command buffers
	//	if (secondaryCmdBufferCount > 0)
	//	{
	//		cmdBuffer->CMD_ExecuteSecondaryCommands(secondaryCmdBuffers.data(), secondaryCmdBufferCount);
	//	}
	//}

	//void Renderer::DrawSkybox(PrimaryCommandBuffer* cmdBuffer)
	//{
	//	auto frameData = GetCurrentFDD();

	//	AssetResources* skyboxAsset = GetAssetResourcesFromUUID(skyboxAssetUUID);
	//	if (skyboxAsset == nullptr)
	//	{
	//		LogError("Skybox asset is not loaded! Failed to draw skybox");
	//		return;
	//	}

	//	SecondaryCommandBuffer* secondaryCmdBuffer = GetSecondaryCommandBufferFromUUID(skyboxAssetUUID);
	//	if (secondaryCmdBuffer == nullptr)
	//	{
	//		return;
	//	}

	//	skyboxPass.UpdateDescriptorSets(currentFrame);

	//	DrawData drawData{};
	//	drawData.asset = skyboxAsset;
	//	drawData.cmdBuffer = secondaryCmdBuffer;
	//	drawData.framebuffer = &frameData->hdrFramebuffer;
	//	drawData.renderPass = &hdrRenderPass;
	//	drawData.framebufferWidth = framebufferWidth;
	//	drawData.framebufferHeight = framebufferHeight;
	//	skyboxPass.Draw(currentFrame, drawData);

	//	VkCommandBuffer vkCmdBuffer = secondaryCmdBuffer->GetBuffer();
	//	cmdBuffer->CMD_ExecuteSecondaryCommands(&vkCmdBuffer, 1);
	//}

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
				LogInfo("Using physical device: '%s'", DeviceCache::Get().GetPhysicalDeviceProperties().deviceName);
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
		bool mailboxValid = false;
		bool fifoValid = false;
		for (const auto& presentMode : availablePresentModes)
		{
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				mailboxValid = true;
			}
			else if (presentMode == VK_PRESENT_MODE_FIFO_KHR)
			{
				fifoValid = true;
			}
		}

		if (CONFIG::EnableVSync)
		{
			if (fifoValid)
			{
				LogInfo("Selected FIFO present mode");
				return VK_PRESENT_MODE_FIFO_KHR;
			}
		}
		else
		{
			if (mailboxValid)
			{
				LogInfo("Selected mailbox present mode");
				return VK_PRESENT_MODE_MAILBOX_KHR;
			}
		}

		// Default to immediate presentation (is this guaranteed to be available?)
		LogInfo("Defaulted to immediate mode presentation");
		return VK_PRESENT_MODE_IMMEDIATE_KHR;
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
			return;
		}

		LogInfo("Selected graphics queue from queue family at index %u"	, indices.GetIndex(QUEUE_TYPE::GRAPHICS));
		LogInfo("Selected compute queue from queue family at index %u"	, indices.GetIndex(QUEUE_TYPE::COMPUTE));
		LogInfo("Selected transfer queue from queue family at index %u"	, indices.GetIndex(QUEUE_TYPE::TRANSFER));
		LogInfo("Selected present queue from queue family at index %u"	, indices.GetIndex(QUEUE_TYPE::PRESENT));

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = {
			indices.GetIndex(QUEUE_TYPE::GRAPHICS),
			indices.GetIndex(QUEUE_TYPE::COMPUTE),
			indices.GetIndex(QUEUE_TYPE::PRESENT),
			indices.GetIndex(QUEUE_TYPE::TRANSFER)
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
		vkGetDeviceQueue(device, indices.GetIndex(QUEUE_TYPE::GRAPHICS)	, 0, &queues[QUEUE_TYPE::GRAPHICS]);
		vkGetDeviceQueue(device, indices.GetIndex(QUEUE_TYPE::COMPUTE)	, 0, &queues[QUEUE_TYPE::COMPUTE ]);
		vkGetDeviceQueue(device, indices.GetIndex(QUEUE_TYPE::TRANSFER)	, 0, &queues[QUEUE_TYPE::TRANSFER]);
		vkGetDeviceQueue(device, indices.GetIndex(QUEUE_TYPE::PRESENT)	, 0, &queues[QUEUE_TYPE::PRESENT ]);
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
		uint32_t queueFamilyIndices[2] = { indices.GetIndex(QUEUE_TYPE::GRAPHICS), indices.GetIndex(QUEUE_TYPE::PRESENT)};

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

		m_swapChainData.resize(imageCount);
		std::vector<VkImage> swapChainImages(imageCount);

		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());

		for (uint32_t i = 0; i < imageCount; i++)
		{
			m_swapChainData[i].swapChainImage.CreateImageViewFromBase(swapChainImages[i], swapChainImageFormat, 1, VK_IMAGE_ASPECT_COLOR_BIT);
		}
		swapChainImages.clear();
	}

	void Renderer::CreateCommandPools()
	{
		CommandPoolRegistry::Get().CreatePools(surface);
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
		//VkFormat depthAttachmentFormat = FindDepthFormat();

		//hdrRenderPass.SetData(VK_FORMAT_R32G32B32A32_SFLOAT, depthAttachmentFormat);
		//hdrRenderPass.Create();

		//ldrRenderPass.SetData(VK_FORMAT_B8G8R8A8_SRGB);
		renderPass.Create();
	}

	void Renderer::CreateFramebuffers()
	{
		auto& swidd = m_swapChainData;

		for (size_t i = 0; i < GetSWIDDSize(); i++)
		{
			std::vector<TextureResource*> attachments =
			{
				&swidd[i].ldrAttachment,
				&swidd[i].swapChainImage
			};

			std::vector<uint32_t> imageViewIndices =
			{
				0,
				0
			};

			FramebufferCreateInfo framebufferInfo{};
			framebufferInfo.renderPass = &renderPass;
			framebufferInfo.attachments = attachments;
			framebufferInfo.imageViewIndices = imageViewIndices;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			swidd[i].swapChainFramebuffer.Create(framebufferInfo);
		}
	}

	//void Renderer::CreateHDRFramebuffers()
	//{
	//	for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
	//	{
	//		auto frameData = GetFrameData(i);

	//		std::vector<TextureResource*> attachments =
	//		{
	//			&(frameData->hdrAttachment),
	//			&(frameData->hdrDepthBuffer)
	//		};

	//		std::vector<uint32_t> imageViewIndices =
	//		{
	//			0,
	//			0
	//		};

	//		FramebufferCreateInfo framebufferInfo{};
	//		framebufferInfo.renderPass = nullptr /*&hdrRenderPass*/;
	//		framebufferInfo.attachments = attachments;
	//		framebufferInfo.imageViewIndices = imageViewIndices;
	//		framebufferInfo.width = swapChainExtent.width;
	//		framebufferInfo.height = swapChainExtent.height;
	//		framebufferInfo.layers = 1;

	//		frameData->hdrFramebuffer.Create(framebufferInfo);
	//	}
	//}

	//void Renderer::CreatePrimaryCommandBuffers()
	//{
	//	for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight(); i++)
	//	{
	//		auto frameData = GetFDDAtIndex(i);

	//		frameData->hdrCommandBuffer.Allocate(QUEUE_TYPE::GRAPHICS);
	//		frameData->postProcessingCommandBuffer.Allocate(QUEUE_TYPE::COMPUTE);
	//		frameData->ldrCommandBuffer.Allocate(QUEUE_TYPE::GRAPHICS);
	//	}
	//}

	void Renderer::CreateSyncObjects()
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		// Creates the fence on the signaled state so we don't block on this fence for
		// the first frame (when we don't have any previous frames to wait on)
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
		{
			auto frameData = GetFrameData(i);

			if (!CreateSemaphore(&(frameData->imageAvailableSemaphore), semaphoreInfo))
			{
				TNG_ASSERT_MSG(false, "Failed to create image available semaphore!");
			}
			if (!CreateSemaphore(&(frameData->renderFinishedSemaphore), semaphoreInfo))
			{
				TNG_ASSERT_MSG(false, "Failed to create render finished semaphore!");
			}
			if (!CreateFence(&(frameData->inFlightFence), fenceInfo))
			{
				TNG_ASSERT_MSG(false, "Failed to create in-flight fence!");
			}
		}
	}

	void Renderer::CreateDescriptorPool()
	{
		// We will create a descriptor pool that can allocate a large number of descriptor sets using the following logic:
		// Since we have to allocate a descriptor set for every unique asset (not sure if this is the correct way, to be honest)
		// and for every frame in flight, we'll set a maximum number of assets (100) and multiply that by the max number of frames
		// in flight
		// TODO - Once I learn how to properly set a different transform for every asset, this will probably have to change...I just
		//        don't know what I'm doing.
		const uint32_t fddSize = CONFIG::MaxFramesInFlight;

		const uint32_t numUniformBuffers = 8;
		const uint32_t numImageSamplers = 7;

		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = numUniformBuffers * CONFIG::MaxFramesInFlight;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = numImageSamplers * CONFIG::MaxFramesInFlight;

		descriptorPool.Create(poolSizes.data(), static_cast<uint32_t>(poolSizes.size()), static_cast<uint32_t>(poolSizes.size()) * fddSize * CONFIG::MaxAssetCount, 0);
	}

	//void Renderer::CreateDepthTextures()
	//{
	//	VkFormat depthFormat = FindDepthFormat();

	//	// HDR depth buffer
	//	BaseImageCreateInfo imageInfo{};
	//	imageInfo.width = framebufferWidth;
	//	imageInfo.height = framebufferHeight;
	//	imageInfo.format = depthFormat;
	//	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	//	imageInfo.mipLevels = 1;
	//	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	//	imageInfo.arrayLayers = 1;
	//	imageInfo.flags = 0;

	//	ImageViewCreateInfo imageViewInfo{};
	//	imageViewInfo.aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
	//	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

	//	SamplerCreateInfo samplerInfo{};
	//	samplerInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	//	samplerInfo.magnificationFilter = VK_FILTER_LINEAR;
	//	samplerInfo.minificationFilter = VK_FILTER_LINEAR;
	//	samplerInfo.enableAnisotropicFiltering = false;
	//	samplerInfo.maxAnisotropy = 1.0f;

	//	for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
	//	{
	//		auto frameData = GetFrameData(i);
	//		frameData->hdrDepthBuffer.Create(&imageInfo, &imageViewInfo, &samplerInfo);
	//	}
	//}

	void Renderer::CreateColorAttachmentTextures()
	{
		// Swap chain color attachment resolve (LDR attachment)
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
		imageViewInfo.viewScope = ImageViewScope::ENTIRE_IMAGE;

		SamplerCreateInfo samplerInfo{};
		samplerInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.magnificationFilter = VK_FILTER_LINEAR;
		samplerInfo.minificationFilter = VK_FILTER_LINEAR;
		samplerInfo.enableAnisotropicFiltering = false;
		samplerInfo.maxAnisotropy = 1.0f;

		for (uint32_t i = 0; i < GetSWIDDSize(); i++)
		{
			auto swidd = GetSWIDDAtIndex(i);
			swidd->ldrAttachment.Create(&imageInfo, &imageViewInfo, &samplerInfo);
		}

		// HDR attachment
		//imageInfo.width = swapChainExtent.width;
		//imageInfo.height = swapChainExtent.height;
		//imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		//imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		//imageInfo.mipLevels = 1;
		//imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		//imageInfo.arrayLayers = 1;
		//imageInfo.flags = 0;

		//samplerInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		//samplerInfo.magnificationFilter = VK_FILTER_LINEAR;
		//samplerInfo.minificationFilter = VK_FILTER_LINEAR;
		//samplerInfo.maxAnisotropy = 1.0f;

		//for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
		//{
		//	auto frameData = GetFrameData(i);
		//	frameData->hdrAttachment.Create(&imageInfo, &imageViewInfo, &samplerInfo);
		//}
	}

	//void Renderer::PerformLDRConversion(PrimaryCommandBuffer* cmdBuffer)
	//{
	//	AssetResources* fullscreenQuadAsset = GetAssetResourcesFromUUID(fullscreenQuadAssetUUID);
	//	if (fullscreenQuadAsset == nullptr)
	//	{
	//		LogError("Fullscreen quad asset is not loaded! Failed to perform LDR conversion");
	//		return;
	//	}

	//	ldrPass.UpdateExposureUniformBuffer(currentFrame, 1.0f);
	//	ldrPass.UpdateDescriptorSets(currentFrame, bloomPass.GetOutputTexture());

	//	DrawData drawData{};
	//	drawData.asset = fullscreenQuadAsset;
	//	drawData.cmdBuffer = cmdBuffer;
	//	drawData.framebuffer = &GetCurrentFDD()->hdrFramebuffer;
	//	drawData.renderPass = &hdrRenderPass;
	//	drawData.framebufferWidth = framebufferWidth;
	//	drawData.framebufferHeight = framebufferHeight;
	//	ldrPass.Draw(currentFrame, drawData);

	//	// NOTE - color attachment is cleared at the beginning of the frame, so transitioning the layout to something
	//	//        else won't make a difference
	//}

	//void Renderer::RecreateAllSecondaryCommandBuffers()
	//{
	//	for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight(); i++)
	//	{
	//		auto frameData = GetFDDAtIndex(i);
	//		for (auto& resources : assetResources)
	//		{
	//			auto iter = frameData->assetCommandBuffers.find(resources.uuid);
	//			if (iter != frameData->assetCommandBuffers.end())
	//			{
	//				SecondaryCommandBuffer* commandBuffer = &iter->second;
	//				commandBuffer->Allocate(QUEUE_TYPE::GRAPHICS);
	//			}
	//		}
	//	}
	//}

	void Renderer::CleanupSwapChain()
	{
		// These are tied to the swapchain only because their size matches the backbuffer
		//for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
		//{
		//	auto frameData = GetFrameData(i);

		//	frameData->hdrAttachment.Destroy();
		//	frameData->hdrDepthBuffer.Destroy();
		//	frameData->hdrFramebuffer.Destroy();
		//}

		for (uint32_t i = 0; i < GetSWIDDSize(); i++)
		{
			auto swidd = GetSWIDDAtIndex(i);

			swidd->ldrAttachment.Destroy();
			swidd->swapChainFramebuffer.Destroy();
			swidd->swapChainImage.DestroyImageViews();
		}

		vkDestroySwapchainKHR(GetLogicalDevice(), swapChain, nullptr);
	}

	VkResult Renderer::SubmitQueue(QUEUE_TYPE type, VkSubmitInfo* info, uint32_t submitCount, VkFence fence, bool waitUntilIdle)
	{
		VkResult res;
		VkQueue queue = queues[type];
		if (queue == VK_NULL_HANDLE)
		{
			TNG_ASSERT_MSG(false, "No queue was found for the given type, failed to submit!");
		}

		res = vkQueueSubmit(queue, submitCount, info, fence);
		if (res != VK_SUCCESS)
		{
			LogError("Failed to submit queue of type %u!", static_cast<uint32_t>(type));
		}

		if (waitUntilIdle)
		{
			res = vkQueueWaitIdle(queue);
			if (res != VK_SUCCESS)
			{
				LogError("Failed to wait until queue of type %u was idle after submitting!", static_cast<uint32_t>(type));
			}
		}

		return res;
	}

	//VkResult Renderer::SubmitCoreRenderingQueue(PrimaryCommandBuffer* cmdBuffer, FrameDependentData* frameData)
	//{
	//	std::array<VkCommandBuffer		, 1> commandBuffers		= { cmdBuffer->GetBuffer()							};
	//	std::array<VkSemaphore			, 1> waitSemaphores		= { frameData->imageAvailableSemaphore				};
	//	std::array<VkSemaphore			, 1> signalSemaphores	= { frameData->coreRenderFinishedSemaphore			};
	//	std::array<VkPipelineStageFlags	, 1> waitStages			= { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT				};

	//	VkSubmitInfo submitInfo{};
	//	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	//	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
	//	submitInfo.pWaitSemaphores = waitSemaphores.data();
	//	submitInfo.pWaitDstStageMask = waitStages.data();
	//	submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
	//	submitInfo.pCommandBuffers = commandBuffers.data();
	//	submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
	//	submitInfo.pSignalSemaphores = signalSemaphores.data();

	//	// Submit the HDR command buffer
	//	return SubmitQueue(cmdBuffer->GetAllocatedQueueType(), &submitInfo, 1);
	//}

	//VkResult Renderer::SubmitPostProcessingQueue(PrimaryCommandBuffer* cmdBuffer, FrameDependentData* frameData)
	//{
	//	std::array<VkCommandBuffer			, 1> commandBuffers		= { cmdBuffer->GetBuffer()							};
	//	std::array<VkSemaphore				, 1> waitSemaphores		= { frameData->coreRenderFinishedSemaphore			};
	//	std::array<VkSemaphore				, 1> signalSemaphores	= { frameData->postProcessingFinishedSemaphore		};
	//	std::array<VkPipelineStageFlags		, 1> waitStages			= { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT				};

	//	VkSubmitInfo submitInfo{};
	//	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	//	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
	//	submitInfo.pWaitSemaphores = waitSemaphores.data();
	//	submitInfo.pWaitDstStageMask = waitStages.data();
	//	submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
	//	submitInfo.pCommandBuffers = commandBuffers.data();
	//	submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
	//	submitInfo.pSignalSemaphores = signalSemaphores.data();

	//	// Submit all compute post-processing effects
	//	return SubmitQueue(cmdBuffer->GetAllocatedQueueType(), &submitInfo, 1);
	//}

	//VkResult Renderer::SubmitLDRConversionQueue(PrimaryCommandBuffer* cmdBuffer, FrameDependentData* frameData)
	//{
	//	std::array<VkCommandBuffer			, 1> commandBuffers		= { cmdBuffer->GetBuffer()						};
	//	std::array<VkSemaphore				, 1> waitSemaphores		= { frameData->postProcessingFinishedSemaphore	};
	//	std::array<VkSemaphore				, 1> signalSemaphores	= { frameData->renderFinishedSemaphore			};
	//	std::array<VkPipelineStageFlags		, 1> waitStages			= { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT			};

	//	VkSubmitInfo submitInfo{};
	//	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	//	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
	//	submitInfo.pWaitSemaphores = waitSemaphores.data();
	//	submitInfo.pWaitDstStageMask = waitStages.data();
	//	submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
	//	submitInfo.pCommandBuffers = commandBuffers.data();
	//	submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
	//	submitInfo.pSignalSemaphores = signalSemaphores.data();

	//	// Submit LDR conversion command buffer. This is the last step in drawing 
	//	// the current frame, so also signal the inFlight fence that the frame is done
	//	return SubmitQueue(cmdBuffer->GetAllocatedQueueType(), &submitInfo, 1, frameData->inFlightFence);
	//}

	//SecondaryCommandBuffer* Renderer::GetSecondaryCommandBufferFromUUID(UUID uuid)
	//{
	//	auto frameData = GetCurrentFDD();

	//	auto secondaryCmdBufferIter = frameData->assetCommandBuffers.find(uuid);
	//	if (secondaryCmdBufferIter == frameData->assetCommandBuffers.end())
	//	{
	//		LogWarning("Asset with UUID '%ull' doesn't have a secondary command buffer?", uuid);
	//		// Should we create a secondary command buffer here?

	//		return nullptr;
	//	}

	//	return &(secondaryCmdBufferIter->second);
	//}

	void Renderer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		DisposableCommand command(QUEUE_TYPE::TRANSFER, true);

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

	/////////////////////////////////////////////////////
	// 
	// Frame data
	//
	/////////////////////////////////////////////////////
	FrameData* Renderer::GetCurrentFrameData()
	{
		return &m_frameData[currentFrame];
	}

	FrameData* Renderer::GetFrameData(uint32_t frameIndex)
	{
		if (frameIndex >= m_frameData.size())
		{
			LogError("Invalid index used to retrieve frame-dependent data");
			return nullptr;
		}

		return &m_frameData[frameIndex];
	}

	/////////////////////////////////////////////////////
	// 
	// Swap-chain image dependent data
	//
	/////////////////////////////////////////////////////

	// Returns the swap-chain image dependent data at the provided index
	Renderer::SwapChainData* Renderer::GetSWIDDAtIndex(uint32_t frameIndex)
	{
		TNG_ASSERT_MSG(frameIndex >= 0 && frameIndex < m_swapChainData.size(), "Invalid index used to retrieve swap-chain image dependent data");
		return &m_swapChainData[frameIndex];
	}

	uint32_t Renderer::GetSWIDDSize() const
	{
		return static_cast<uint32_t>(m_swapChainData.size());
	}

}