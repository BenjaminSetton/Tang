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
//
//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#define GLM_FORCE_ALIGNED_GENTYPES
//#include <glm.hpp>
//#include <gtc/matrix_transform.hpp>
//
//#define GLM_ENABLE_EXPERIMENTAL
//#include <gtx/hash.hpp>
//
//#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>

//#include <array>
//#include <algorithm>
//#include <chrono>
//#include <cstdlib>
//#include <cstdint>
//#include <fstream>
//#include <glm.hpp>
//#include <iostream>
//#include <optional>
//#include <set>
//#include <stdexcept>
//#include <unordered_map>
#include <vector>


//#include "asset_loader.h"
#include "asset_types.h"
#include "buffer/vertex_buffer.h"
#include "buffer/index_buffer.h"
#include "utils/sanity_check.h"

struct QueueFamilyIndices;
struct SwapChainSupportDetails;
struct UniformBufferObject;

struct AssetResources
{
	std::vector<TANG::VertexBuffer> vertexBuffers;
	std::vector<TANG::IndexBuffer> indexBuffers;
	uint64_t numIndices = 0;							// Used when calling vkCmdDrawIndexed
	void* assetPtr = nullptr;							// Used to track what asset these resources were created on. In good theory this pointer should never
														// change as long as the asset is "alive", since the pointer is retrieved from the AssetContainer
};

namespace TANG
{
	class Renderer {
	public:

		void Initialize();

		void Update();

		void Shutdown();

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

		void drawFrame();

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

		bool isDeviceSuitable(VkPhysicalDevice device);

		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

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
		void createLogicalDevice();

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

		void createCommandPool();

		void createCommandBuffers();

		void createSyncObjects();

		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

		void createDescriptorSetLayout();

		void createUniformBuffers();

		void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format,
			VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
			VkImage& image, VkDeviceMemory& imageMemory);

		void createTextureImage();

		void createDescriptorPool();

		void createDescriptorSets();

		VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

		void createTextureImageView();

		void createTextureSampler();

		void createDepthResources();

		void createColorResources();

		void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

		void recreateSwapChain();

		void cleanupSwapChain();

		void updateUniformBuffer(uint32_t currentFrame);

		VkCommandBuffer BeginSingleTimeCommands();

		void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

		VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		VkFormat findDepthFormat();

		bool hasStencilComponent(VkFormat format);

		void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

		VkSampleCountFlagBits getMaxUsableSampleCount();

		void DestroyAssetBuffersHelper(AssetResources& resources);

	private:

		GLFWwindow* windowHandle = nullptr;

		VkInstance vkInstance;
		VkDebugUtilsMessengerEXT debugMessenger;

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice logicalDevice;

		VkSurfaceKHR surface;

		VkQueue graphicsQueue;
		VkQueue presentQueue;

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

		VkCommandPool commandPool;
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
	};

}

#endif