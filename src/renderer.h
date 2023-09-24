#ifndef RENDERER_H
#define RENDERER_H

//
// This code was taken from the Vulkan tutorial website:
// https://vulkan-tutorial.com
//

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3native.h>

#include <unordered_map>
#include <vector>

#include "asset_types.h"
#include "cmd_buffer/primary_command_buffer.h"
#include "cmd_buffer/secondary_command_buffer.h"
#include "utils/sanity_check.h"

namespace TANG
{
	struct QueueFamilyIndices;
	struct SwapChainSupportDetails;
	struct UniformBufferObject;

	enum QueueType
	{
		GRAPHICS_QUEUE,
		PRESENT_QUEUE,
		TRANSFER_QUEUE,
		QUEUE_COUNT			// NOTE! This value must come last at all times!! This is used to count the number of values inside this enum
	};

	class Renderer {
	public:

		void Initialize();

		void Update();

		void Shutdown();

		// Helper functions for setting and retrieving the asset draw state of a particular asset.
		// The asset draw state is cleared every frame, so SetAssetDrawState must be called on a
		// per-frame basis to draw assets. In other words, assets will not be drawn unless SetAssetDrawState()
		// is explicitly called that frame
		void SetAssetDrawState(UUID uuid);
		bool GetAssetDrawState(UUID uuid);

		// Loads an asset which implies grabbing the vertices and indices from the asset container
		// and creating vertex/index buffers to contain them. It also includes creating all other
		// API objects necessary for rendering.
		// 
		// Before calling this function, make sure you've called LoaderUtils::LoadAsset() and have
		// successfully loaded an asset from file! This functions assumes this, and if it can't retrieve
		// the loaded asset data it will return prematurely
		AssetResources* CreateAssetResources(AssetCore* asset);

		// Creates a secondary command buffer, given the asset resources. After an asset is loaded and it's asset resources
		// are loaded, this function must be called to create the secondary command buffer that holds the commands to render
		// the asset.
		void CreateAssetCommandBuffer(AssetResources* resources);

		void DestroyAssetResources(AssetCore* asset);
		void DestroyAllAssetResources();

		// TODO - Abstract this out into a window class, this really shouldn't be part of the renderer
		bool WindowShouldClose();

		bool framebufferResized = false;

	private:

		static void framebufferResizeCallback(GLFWwindow* windowHandle, int width, int height)
		{
			UNUSED(width);
			UNUSED(height);

			auto app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(windowHandle));
			app->framebufferResized = true;
		}

		// NOTE - The window creation and management using GLFW will be abstracted away from this class in the future.
		//        The renderer should only be in charge of initializing, maintaining and destroying Vulkan-related objects.
		void InitWindow();
		void InitVulkan();

		void ShutdownWindow();
		void ShutdownVulkan();

		void DrawFrame();

		void createInstance();

		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

		void setupDebugMessenger();

		std::vector<const char*> getRequiredExtensions();

		bool checkValidationLayerSupport();

		bool checkDeviceExtensionSupport(VkPhysicalDevice device);

		////////////////////////////////////////
		//
		//  PHYSICAL DEVICE
		//
		////////////////////////////////////////
		void pickPhysicalDevice();

		bool IsDeviceSuitable(VkPhysicalDevice device);

		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		////////////////////////////////////////
		//
		//  LOGICAL DEVICE
		//
		////////////////////////////////////////
		void CreateLogicalDevice();

		////////////////////////////////////////
		//
		//  SURFACE
		//
		////////////////////////////////////////
		void createSurface();

		void CreateSwapChain();

		// Create image views for all images on the swap chain
		void CreateImageViews();

		// This is a helper function for creating the "VkShaderModule" wrappers around
		// the shader code, read from createGraphicsPipeline() below
		VkShaderModule createShaderModule(std::vector<char>& code);

		void CreateGraphicsPipeline();

		void CreateRenderPass();

		void CreateFramebuffers();

		void CreateCommandPools();

		void CreatePrimaryCommandBuffers(QueueType poolType);

		void createSyncObjects();

		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

		void createDescriptorSetLayout();

		void createUniformBuffers();

		void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format,
			VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
			VkImage& image, VkDeviceMemory& imageMemory);

		void CreateTextureImage();

		void createDescriptorPool();

		void createDescriptorSets();

		VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

		void createTextureImageView();

		void createTextureSampler();

		void CreateDepthResources();

		void CreateColorResources();

		void RecordPrimaryCommandBuffer(uint32_t frameBufferIndex);
		void RecordSecondaryCommandBuffer(SecondaryCommandBuffer& commandBuffer, AssetResources* resources, uint32_t frameBufferIndex);

		void RecreateSwapChain();

		void RecreateAllSecondaryCommandBuffers();

		void CleanupSwapChain();

		void updateUniformBuffer(uint32_t currentFrame);

		VkCommandBuffer BeginSingleTimeCommands(VkCommandPool pool);

		// The commandPoolType parameter must match the pool type that was used to allocate the command buffer in the corresponding BeginSingleTimeCommands() function call!
		void EndSingleTimeCommands(VkCommandBuffer commandBuffer, QueueType commandPoolType);

		void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

		VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		VkFormat findDepthFormat();

		bool hasStencilComponent(VkFormat format);

		void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

		VkSampleCountFlagBits getMaxUsableSampleCount();

		void DestroyAssetBuffersHelper(AssetResources& resources);

		PrimaryCommandBuffer* GetCurrentPrimaryBuffer();
		SecondaryCommandBuffer* GetCurrentSecondaryCommandBuffer(uint32_t frameBufferIndex, UUID uuid);
		VkFramebuffer GetCurrentFramebuffer(uint32_t frameBufferIndex) const;
		VkDescriptorSet GetCurrentDescriptorSet() const;

	private:

		GLFWwindow* windowHandle = nullptr;

		VkInstance vkInstance;
		VkDebugUtilsMessengerEXT debugMessenger;

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice logicalDevice;

		VkSurfaceKHR surface;

		std::unordered_map<QueueType, VkQueue> queues;

		VkSwapchainKHR swapChain;
		std::vector<VkImage> swapChainImages;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;
		std::vector<VkImageView> swapChainImageViews;

		VkRenderPass renderPass;
		VkDescriptorSetLayout descriptorSetLayout;
		VkPipelineLayout pipelineLayout;

		VkPipeline graphicsPipeline;

		std::vector<VkFramebuffer> swapChainFramebuffers;
		uint32_t swapChainImageCount = 0; // NOTE - This does not necessarily equal the number of frames in flight!

		std::unordered_map<QueueType, VkCommandPool> commandPools;

		// We need one primary command buffer per frame in flight, since we can be rendering multiple frames at the same time and
		// we want to still be able to reset and record a primary buffer
		std::vector<PrimaryCommandBuffer> primaryCommandBuffers;

		// The two following maps represent the drawable state of an asset, given it's UUID. AssetDrawStates tells us whether the asset must
		// be drawn this frame, and secondaryCommandBuffers holds a correspondence between an asset's UUID and it's generated secondary command buffer
		std::unordered_map<UUID, bool> assetDrawStates;
		// This is a vector of unordered maps because we must bind the current frame's framebuffer in the secondary command buffers, so instead of recording
		// them every frame when nothing changes but the framebuffer, we'll create the same number of maps as there are frames in flight
		std::vector<std::unordered_map<UUID, SecondaryCommandBuffer>> secondaryCommandBuffers;

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;

		uint32_t currentFrame = 0;

		std::vector<AssetResources> assetResources;

		std::vector<VkBuffer> uniformBuffers;
		std::vector<VkDeviceMemory> uniformBufferMemory;
		std::vector<void*> uniformBuffersMapped;

		VkDescriptorPool descriptorPool;
		std::vector<VkDescriptorSet> descriptorSets;

		uint32_t mipLevels;
		VkImage textureImage;
		VkDeviceMemory textureImageMemory;
		VkImageView textureImageView;
		VkSampler textureSampler;

		VkImage depthImage;
		VkDeviceMemory depthImageMemory;
		VkImageView depthImageView;

		// Multisampled anti-aliasing
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
		VkImage colorImage;
		VkDeviceMemory colorImageMemory;
		VkImageView colorImageView;

	};

}

#endif