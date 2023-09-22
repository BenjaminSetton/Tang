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
#include "buffer/vertex_buffer.h"
#include "buffer/index_buffer.h"
#include "utils/sanity_check.h"
#include "utils/uuid.h"

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

	struct AssetResources
	{
		std::vector<TANG::VertexBuffer> vertexBuffers;
		std::vector<TANG::IndexBuffer> indexBuffers;
		uint64_t numIndices = 0;							// Used when calling vkCmdDrawIndexed
		UUID uuid;
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
		void CreateAssetResources(Asset* asset);

		void DestroyAssetResources(Asset* asset);
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

		// Separates the creation of GLFW window objects and Vulkan graphics objects
		void InitWindow();
		void InitVulkan();

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

		void createSwapChain();

		// Create image views for all images on the swap chain
		void createImageViews();

		// This is a helper function for creating the "VkShaderModule" wrappers around
		// the shader code, read from createGraphicsPipeline() below
		VkShaderModule createShaderModule(std::vector<char>& code);

		void createGraphicsPipeline();

		void createRenderPass();

		void createFramebuffers();

		void CreateCommandPools();

		void CreateCommandBuffers(QueueType poolType);

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

		void createDepthResources();

		void createColorResources();

		bool RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

		void recreateSwapChain();

		void cleanupSwapChain();

		void updateUniformBuffer(uint32_t currentFrame);

		VkCommandBuffer BeginSingleTimeCommands(VkCommandPool pool);

		// The commandPoolType parameter must match the pool type that was used to allocate the command buffer in the corresponding BeginSingleTimeCommands() function call!
		void EndSingleTimeCommands(VkCommandBuffer commandBuffer, QueueType commandPoolType);

		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

		VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		VkFormat findDepthFormat();

		bool hasStencilComponent(VkFormat format);

		void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

		VkSampleCountFlagBits getMaxUsableSampleCount();

		void DestroyAssetBuffersHelper(AssetResources& resources);

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

		std::unordered_map<QueueType, VkCommandPool> commandPools;
		std::vector<VkCommandBuffer> commandBuffers;

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

		std::unordered_map<UUID, bool> assetDrawStates;
	};

}

#endif