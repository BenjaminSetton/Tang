#ifndef RENDERER_H
#define RENDERER_H

#include <array>
#include <optional>
#include <queue>
#include <unordered_map>
#include <vector>

#include "cmd_buffer/primary_command_buffer.h"
#include "cmd_buffer/secondary_command_buffer.h"

#include "data_buffer/uniform_buffer.h"

#include "descriptors/descriptor_pool.h"
#include "descriptors/descriptor_set.h"
#include "descriptors/set_layout/set_layout_cache.h"
#include "descriptors/set_layout/set_layout_summary.h"

#include "render_passes/hdr_render_pass.h"
#include "render_passes/ldr_render_pass.h"

#include "callback_types.h"
#include "config.h"
#include "frame_data.h"
#include "framebuffer.h"
#include "queue_types.h"
#include "texture_resource.h"

struct GLFWwindow;

namespace TANG
{
	// Forward declarations
	struct SwapChainSupportDetails;
	class QueueFamilyIndices;
	class DisposableCommand;

	class Renderer
	{

	private:

		Renderer();
		Renderer(const Renderer& other) = delete;
		Renderer& operator=(const Renderer& other) = delete;

		friend class DisposableCommand;

	public:

		static Renderer& GetInstance()
		{
			static Renderer instance;
			return instance;
		}

		void Initialize(GLFWwindow* windowHandle, uint32_t windowWidth, uint32_t windowHeight);

		// Core update loop for the renderer
		void Update(float deltaTime);

		// These begin/end frame calls are used to control the render context. Once a frame is initiated,
		// all subsequent draw calls must refer to the RenderContext for information about the current frame
		// since every Vulkan API call must be relative to that. EndFrame() simply invalidates the current
		// render context and creates a new one for the next frame
		void BeginFrame();
		void EndFrame();

		// The core draw call. Conventionally, the state of the renderer must be updated through a call to Update() before this call is made
		void Draw();

		// Releases all internal handles to Vulkan objects
		void Shutdown();

		// TODO - Maybe move all allocation calls to a Device class (DeviceCache?)
		// Allocates a descriptor set with the given description through one of the renderer's internal descriptor pools
		[[nodiscard]] DescriptorSet AllocateDescriptorSet(const DescriptorSetLayout& setLayout);

		// TODO - Maybe move all allocation calls to a Device class (DeviceCache?)
		// Allocates a primary or secondary command buffer from the provided queue type (graphics, present, compute, etc queue)
		[[nodiscard]] PrimaryCommandBuffer AllocatePrimaryCommandBuffer(QUEUE_TYPE type);
		[[nodiscard]] SecondaryCommandBuffer AllocateSecondaryCommandBuffer(QUEUE_TYPE type);

		// Create sync objects. Returns true if sync object was successfully created, false otherwise
		bool CreateSemaphore(VkSemaphore* semaphore, const VkSemaphoreCreateInfo& createInfo);
		void DestroySemaphore(VkSemaphore* semaphore);

		bool CreateFence(VkFence* fence, const VkFenceCreateInfo& createInfo);
		void DestroyFence(VkFence* fence);

		// Appends the provided command buffer to the rendering queue, unless command buffer pointer is null. Queue order is preserved when drawing
		void QueueCommandBuffer(const PrimaryCommandBuffer& cmd, const QueueSubmitInfo& info);

		// Sets the draw state of the given asset to TRUE.
		// The asset draw state is cleared every frame, so SetAssetDrawState() must be called on a
		// per-frame basis to draw assets. In other words, assets will not be drawn unless SetAssetDrawState()
		// is explicitly called that frame.
		// NOTE - No getter is defined on purpose, the data should only be received from the API and kept in the renderer
		//void SetAssetDrawState(UUID uuid);

		// The following functions provide different ways of modifying the internal transform data of the provided asset
		// NOTE - No getters are defined on purpose, the data should only be received from the API and kept in the renderer
		//void SetAssetTransform(UUID uuid, const Transform& transform);
		//void SetAssetPosition(UUID uuid, const glm::vec3& position);
		//void SetAssetRotation(UUID uuid, const glm::vec3& rotation);
		//void SetAssetScale(UUID uuid, const glm::vec3& scale);

		// Sets the size that the next framebuffer should be. This function will only be called when the main window is resized
		void SetNextFramebufferSize(uint32_t newWidth, uint32_t newHeight);
		Framebuffer* GetCurrentSwapChainFramebuffer();

		// Updates the view/proj matrix using the provided position and inverted view matrix. The caller can get this data from any derived BaseCamera object
		//void UpdateCameraData(const glm::vec3& position, const glm::mat4& viewMatrix, const glm::mat4& proj);

		VkFormat FindDepthFormat();

		bool HasStencilComponent(VkFormat format);

		void WaitForFence(VkFence fence, uint64_t timeout);

		uint32_t GetCurrentFrameIndex() const;

		VkSemaphore GetCurrentImageAvailableSemaphore() const;
		VkSemaphore GetCurrentRenderFinishedSemaphore() const;
		VkFence GetCurrentFrameFence() const;

		void RegisterSwapChainRecreatedCallback(SwapChainRecreatedCallback callback);
		void RegisterRendererShutdownCallback(RendererShutdownCallback callback);

	private:

		VkInstance vkInstance;
		VkDebugUtilsMessengerEXT debugMessenger;

		VkSurfaceKHR surface;

		std::unordered_map<QUEUE_TYPE, VkQueue> queues;

		VkSwapchainKHR swapChain;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;

		SwapChainRecreatedCallback m_swapChainRecreatedCallback;
		RendererShutdownCallback m_rendererShutdownCallback;

		////////////////////////////////////////////////////////////////////
		// 
		//	FRAME-DEPENDENT DATA
		//
		//	Organizes data that depends on the maximum number of frames in flight
		// 
		////////////////////////////////////////////////////////////////////
		//struct FrameDependentData
		//{
		//	// Sync objects
		//	VkSemaphore imageAvailableSemaphore;			// Signalled when swap-chain image becomes available
		//	VkSemaphore coreRenderFinishedSemaphore;		// Signalled when the core render loop is finished
		//	VkSemaphore postProcessingFinishedSemaphore;	// Signalled when image post-processing is finished
		//	VkSemaphore renderFinishedSemaphore;			// Signalled when the frame's rendering is finished (includes core pass and post-processing)
		//	VkFence inFlightFence;

		//	// We need one primary command buffer per frame in flight, since we can be rendering multiple frames at the same time and
		//	// we want to still be able to reset and record a primary buffer
		//	//PrimaryCommandBuffer hdrCommandBuffer;
		//	//PrimaryCommandBuffer postProcessingCommandBuffer;
		//	//PrimaryCommandBuffer ldrCommandBuffer;
		//	//std::unordered_map<UUID, SecondaryCommandBuffer> assetCommandBuffers;

		//	TextureResource hdrDepthBuffer;
		//	TextureResource hdrAttachment;
		//	Framebuffer hdrFramebuffer;
		//};
		//std::vector<FrameDependentData> frameDependentData;

		////////////////////////////////////////////////////////////////////
		// 
		//	SWAP-CHAIN IMAGE-DEPENDENT DATA
		//
		//	Organizes data that depends on the number of images in the swap chain, which may differ from the number of frames in flight
		// 
		////////////////////////////////////////////////////////////////////
		struct SwapChainData
		{
			TextureResource ldrAttachment;
			TextureResource swapChainImage;
			Framebuffer swapChainFramebuffer;
		};
		std::vector<SwapChainData> m_swapChainData;

		//BloomPass bloomPass;
		//SkyboxPass skyboxPass;
		//CubemapPreprocessingPass cubemapPreprocessingPass;
		//PBRPass pbrPass;
		//LDRPass ldrPass;

		// Holds a queue of all the command queues that are to be submitted this frame. Queue is consumed in Draw()
		std::queue<std::pair<PrimaryCommandBuffer, QueueSubmitInfo>> m_cmdQueuesToSubmit;

		std::array<FrameData, CONFIG::MaxFramesInFlight> m_frameData;
		uint32_t currentFrame;

		//HDRRenderPass hdrRenderPass;
		LDRRenderPass renderPass;

		//UUID skyboxAssetUUID;
		//UUID fullscreenQuadAssetUUID;

		// TODO - Move to it's own manager
		DescriptorPool descriptorPool;

		// Cached window sizes
		uint32_t framebufferWidth, framebufferHeight;

	private:

		void CreateSurface(GLFWwindow* windowHandle);

		void CreateInstance();

		//void DrawFrame();

		//void DrawAssets(PrimaryCommandBuffer* cmdBuffer);
		//void DrawSkybox(PrimaryCommandBuffer* cmdBuffer);

		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

		void SetupDebugMessenger();

		std::vector<const char*> GetRequiredExtensions();

		bool CheckValidationLayerSupport();

		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

		void PickPhysicalDevice();

		bool IsDeviceSuitable(VkPhysicalDevice device);

		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

		VkExtent2D ChooseSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t actualWidth, uint32_t actualHeight);

		void CreateLogicalDevice();

		void CreateSwapChain();
		void CreateSwapChainImageViews(uint32_t imageCount);

		// Creates a secondary command buffer, given the asset resources. After an asset is loaded and it's asset resources
		// are loaded, this function must be called to create the secondary command buffer that holds the commands to render
		// the asset.
		//void CreateAssetCommandBuffer(AssetResources* resources);

		void CreateCommandPools();

		void CreateRenderPasses();

		void CreateFramebuffers();
		//void CreateHDRFramebuffers();

		//void CreatePrimaryCommandBuffers();

		void CreateSyncObjects();

		void CreateDescriptorPool();

		//void CreateDepthTextures();
		void CreateColorAttachmentTextures();

		//void PerformLDRConversion(PrimaryCommandBuffer* cmdBuffer);

		//void RecreateAllSecondaryCommandBuffers();

		void RecreateSwapChain();

		void CleanupSwapChain();

		// Submits the provided queue type, along with the provided command buffer. Return value should _not_ be ignored
		[[nodiscard]] VkResult SubmitQueue(QUEUE_TYPE type, VkSubmitInfo* info, uint32_t submitCount, VkFence fence = VK_NULL_HANDLE, bool waitUntilIdle = false);
		//[[nodiscard]] VkResult SubmitCoreRenderingQueue(PrimaryCommandBuffer* cmdBuffer, FrameDependentData* frameData);
		//[[nodiscard]] VkResult SubmitPostProcessingQueue(PrimaryCommandBuffer* cmdBuffer, FrameDependentData* frameData);
		//[[nodiscard]] VkResult SubmitLDRConversionQueue(PrimaryCommandBuffer* cmdBuffer, FrameDependentData* frameData);

		//AssetResources* GetAssetResourcesFromUUID(UUID uuid);
		//SecondaryCommandBuffer* GetSecondaryCommandBufferFromUUID(UUID uuid);

		void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

		VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		VkFramebuffer GetFramebufferAtIndex(uint32_t frameBufferIndex);

		// Returns the current frame-dependent data
		FrameData* GetCurrentFrameData();

		// Returns the frame-dependent data at the provided index
		FrameData* GetFrameData(uint32_t frameIndex);

		// Returns the swap-chain image dependent data at the provided index
		SwapChainData* GetSWIDDAtIndex(uint32_t frameIndex);

		// Returns the size of the swap-chain image-dependent data vector. This entirely depends on the number of images that are generated for the swap-chain
		uint32_t GetSWIDDSize() const;

	};

}

#endif