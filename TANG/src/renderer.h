#ifndef RENDERER_H
#define RENDERER_H

#include <array>
#include <optional>
#include <unordered_map>
#include <vector>

#include "asset_types.h"

#include "cmd_buffer/primary_command_buffer.h"
#include "cmd_buffer/secondary_command_buffer.h"

#include "data_buffer/uniform_buffer.h"

#include "descriptors/descriptor_pool.h"
#include "descriptors/descriptor_set.h"
#include "descriptors/set_layout/set_layout_cache.h"
#include "descriptors/set_layout/set_layout_summary.h"

#include "passes/bloom_pass.h"
#include "passes/cubemap_preprocessing_pass.h"
#include "passes/pbr_pass.h"
#include "passes/skybox_pass.h"

#include "pipelines/ldr_pipeline.h"
#include "pipelines/pbr_pipeline.h"

#include "render_passes/hdr_render_pass.h"
#include "render_passes/ldr_render_pass.h"

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

		// The core draw call. Conventionally, the state of the renderer must be updated through a call to Update() before this call is made
		void Draw();

		// Releases all internal handles to Vulkan objects
		void Shutdown();

		// Sets the draw state of the given asset to TRUE.
		// The asset draw state is cleared every frame, so SetAssetDrawState() must be called on a
		// per-frame basis to draw assets. In other words, assets will not be drawn unless SetAssetDrawState()
		// is explicitly called that frame.
		// NOTE - No getter is defined on purpose, the data should only be received from the API and kept in the renderer
		void SetAssetDrawState(UUID uuid);

		// The following functions provide different ways of modifying the internal transform data of the provided asset
		// NOTE - No getters are defined on purpose, the data should only be received from the API and kept in the renderer
		void SetAssetTransform(UUID uuid, const Transform& transform);
		void SetAssetPosition(UUID uuid, const glm::vec3& position);
		void SetAssetRotation(UUID uuid, const glm::vec3& rotation);
		void SetAssetScale(UUID uuid, const glm::vec3& scale);

		// Loads an asset which implies grabbing the vertices and indices from the asset container
		// and creating vertex/index buffers to contain them. It also includes creating all other
		// API objects necessary for rendering. This resources created depend entirely on the pipeline
		// 
		// Before calling this function, make sure you've called LoaderUtils::LoadAsset() and have
		// successfully loaded an asset from file! This functions assumes this, and if it can't retrieve
		// the loaded asset data it will return prematurely
		AssetResources* CreateAssetResources(AssetDisk* asset, CorePipeline corePipeline);

		void CreatePBRAssetResources(AssetDisk* asset, AssetResources& out_resources);
		void CreateSkyboxAssetResources(AssetDisk* asset, AssetResources& out_resources);
		void CreateFullscreenQuadAssetResources(AssetDisk* asset, AssetResources& out_resources);

		void DestroyAssetResources(UUID uuid);
		void DestroyAllAssetResources();

		// Sets the size that the next framebuffer should be. This function will only be called when the main window is resized
		void SetNextFramebufferSize(uint32_t newWidth, uint32_t newHeight);

		// Updates the view matrix using the provided position and inverted view matrix. The caller can get this data from any derived BaseCamera object
		void UpdateCameraData(const glm::vec3& position, const glm::mat4& viewMatrix);

	private:

		VkInstance vkInstance;
		VkDebugUtilsMessengerEXT debugMessenger;

		VkSurfaceKHR surface;

		std::unordered_map<QueueType, VkQueue> queues;

		VkSwapchainKHR swapChain;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;

		// Stores all the data we need to describe an asset. We need to have a vector of descriptor sets per asset
		// because we divide the descriptor sets based on how frequently they're updated. For example the asset's
		// position might change every frame, but the PBR textures will likely seldom change (if at all)
		struct AssetDescriptorData
		{
			std::vector<DescriptorSet> descriptorSets;

			UniformBuffer transformUBO;
		};


		////////////////////////////////////////////////////////////////////
		// 
		//	FRAME-DEPENDENT DATA
		//
		//	Organizes data that depends on the maximum number of frames in flight
		// 
		////////////////////////////////////////////////////////////////////
		struct FrameDependentData
		{
			// Assets
			std::unordered_map<UUID, AssetDescriptorData> assetDescriptorDataMap;

			UniformBuffer viewUBO;
			UniformBuffer projUBO;
			UniformBuffer cameraDataUBO;

			// Skybox
			std::array<DescriptorSet, 3> skyboxDescriptorSets;

			// LDR
			UniformBuffer ldrCameraDataUBO;
			DescriptorSet ldrDescriptorSet;

			// Sync objects
			VkSemaphore imageAvailableSemaphore;			// Signalled when swap-chain image becomes available
			VkSemaphore coreRenderFinishedSemaphore;		// Signalled when the core render loop is finished
			VkSemaphore postProcessingFinishedSemaphore;	// Signalled when image post-processing is finished
			VkSemaphore renderFinishedSemaphore;			// Signalled when the frame's rendering is finished (includes core pass and post-processing)
			VkFence inFlightFence;

			// We need one primary command buffer per frame in flight, since we can be rendering multiple frames at the same time and
			// we want to still be able to reset and record a primary buffer
			PrimaryCommandBuffer hdrCommandBuffer;
			PrimaryCommandBuffer postProcessingCommandBuffer;
			PrimaryCommandBuffer ldrCommandBuffer;
			std::unordered_map<UUID, SecondaryCommandBuffer> assetCommandBuffers;

			TextureResource hdrDepthBuffer;
			TextureResource hdrAttachment;
			Framebuffer hdrFramebuffer;
		};
		std::vector<FrameDependentData> frameDependentData;
		// We want to organize our descriptor sets as follows:
		// 
		// FOR EVERY ASSET:
		//		FOR EVERY FRAME IN FLIGHT:
		//			Descriptor set 0:
		//				- diffuse sampler			(binding 0)
		//				- normal sampler			(binding 1)
		//				- metallic sampler			(binding 2)
		//				- roughness sampler			(binding 3)
		//				- lightmap sampler			(binding 4)
		//			Descriptor set 1:
		//				- Projection matrix UBO		(binding 0)
		//			Descriptor set 2:
		//				- CameraData UBO			(binding 0)
		//				- Transform matrix UBO		(binding 1)
		//				- View matrix UBO			(binding 2)
		// 
		// Total per frame in flight: 3 descriptor sets - 4 uniform buffers and 5 image samplers
		// Total per all frames in flight (2): 6 descriptor sets - 8 uniform buffers and 10 image samplers


		////////////////////////////////////////////////////////////////////
		// 
		//	SWAP-CHAIN IMAGE-DEPENDENT DATA
		//
		//	Organizes data that depends on the number of images in the swap chain, which may differ from the number of frames in flight
		// 
		////////////////////////////////////////////////////////////////////
		struct SwapChainImageDependentData
		{
			TextureResource ldrAttachment;
			TextureResource swapChainImage;
			Framebuffer swapChainFramebuffer;
		};
		std::vector<SwapChainImageDependentData> swapChainImageDependentData;


		PBRPipeline pbrPipeline;
		SetLayoutCache pbrSetLayoutCache;

		BloomPass bloomPass;
		SkyboxPass skyboxPass;
		CubemapPreprocessingPass cubemapPreprocessingPass;
		PBRPass pbrPass;

		HDRRenderPass hdrRenderPass;

		LDRRenderPass ldrRenderPass;
		LDRPipeline ldrPipeline;
		SetLayoutCache ldrSetLayoutCache;

		UUID skyboxAssetUUID;
		UUID fullscreenQuadAssetUUID;

		uint32_t currentFrame;

		// TODO - Rework this garbage
		glm::vec3 startingCameraPosition;
		glm::mat4 startingCameraViewMatrix;
		glm::mat4 startingProjectionMatrix;

		// The assetResources vector contains all the vital information that we need for every asset in order to render it
		// The resourcesMap maps an asset's UUID to a location within the assetResources vector
		std::unordered_map<UUID, uint32_t> resourcesMap;
		std::vector<AssetResources> assetResources;

		DescriptorPool descriptorPool;

		// Cached window sizes
		uint32_t framebufferWidth, framebufferHeight;

	private:

		void CreateSurface(GLFWwindow* windowHandle);

		void CreateInstance();

		void DrawFrame();

		void DrawSkybox(PrimaryCommandBuffer* cmdBuffer);

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
		void CreateAssetCommandBuffer(AssetResources* resources);

		void CreateCommandPools();

		void CreatePipelines();

		void CreateRenderPasses();

		void CreateFramebuffers();
		void CreateLDRFramebuffers();
		void CreateHDRFramebuffers();

		void CreatePrimaryCommandBuffers();

		void CreateSyncObjects();

		void CreateAssetUniformBuffers(UUID uuid);
		void CreateFrameUniformBuffers();
		void CreateLDRUniformBuffer();

		void CreateAssetDescriptorSets(UUID uuid);
		void CreateLDRDescriptorSet();

		void CreateDescriptorSetLayouts();
		void CreatePBRSetLayouts();
		void CreateLDRSetLayouts();

		void CreateDescriptorPool();

		void CreateDepthTextures();
		void CreateColorAttachmentTextures();

		void DrawAssets(PrimaryCommandBuffer* cmdBuffer);
		void RecordSecondaryCommandBuffer(SecondaryCommandBuffer* cmdBuffer, AssetResources* resources);

		void PerformLDRConversion(PrimaryCommandBuffer* cmdBuffer);

		void RecreateAllSecondaryCommandBuffers();

		void RecreateSwapChain();

		void CleanupSwapChain();

		void InitializeDescriptorSets(UUID uuid, uint32_t frameIndex);
		void InitializeFrameUniformBuffers();

		void UpdateTransformDescriptorSet(UUID uuid);
		void UpdateCameraDataDescriptorSet(UUID uuid, uint32_t frameIndex);
		void UpdateProjectionDescriptorSet(UUID uuid, uint32_t frameIndex);
		void UpdatePBRTextureDescriptorSet(UUID uuid, uint32_t frameIndex);
		void UpdateLDRDescriptorSet();

		void UpdateTransformUniformBuffer(const Transform& transform, UUID uuid);
		void UpdateCameraDataUniformBuffers(uint32_t frameIndex, const glm::vec3& position, const glm::mat4& viewMatrix);
		void UpdateProjectionUniformBuffer(uint32_t frameIndex);
		void UpdateLDRUniformBuffer();

		// Submits the provided queue type, along with the provided command buffer. Return value should _not_ be ignored
		[[nodiscard]] VkResult SubmitQueue(QueueType type, VkSubmitInfo* info, uint32_t submitCount, VkFence fence = VK_NULL_HANDLE, bool waitUntilIdle = false);
		[[nodiscard]] VkResult SubmitCoreRenderingQueue(CommandBuffer* cmdBuffer, FrameDependentData* frameData);
		[[nodiscard]] VkResult SubmitPostProcessingQueue(CommandBuffer* cmdBuffer, FrameDependentData* frameData);
		[[nodiscard]] VkResult SubmitLDRConversionQueue(CommandBuffer* cmdBuffer, FrameDependentData* frameData);

		AssetResources* GetAssetResourcesFromUUID(UUID uuid);
		SecondaryCommandBuffer* GetSecondaryCommandBufferFromUUID(UUID uuid);

		void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

		VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		VkFormat FindDepthFormat();

		bool HasStencilComponent(VkFormat format);

		void DestroyAssetBuffersHelper(AssetResources* resources);

		VkFramebuffer GetFramebufferAtIndex(uint32_t frameBufferIndex);

		// Returns the current frame-dependent data
		FrameDependentData* GetCurrentFDD();

		// Returns the frame-dependent data at the provided index
		FrameDependentData* GetFDDAtIndex(uint32_t frameIndex);

		// Returns the size of the frame-dependent data vector. This is equivalent to MAX_FRAMES_IN_FLIGHT
		uint32_t GetFDDSize() const;

		// Returns the swap-chain image dependent data at the provided index
		SwapChainImageDependentData* GetSWIDDAtIndex(uint32_t frameIndex);

		// Returns the size of the swap-chain image-dependent data vector. This entirely depends on the number of images that are generated for the swap-chain
		uint32_t GetSWIDDSize() const;

	};

}

#endif