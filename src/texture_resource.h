#ifndef TEXTURE_RESOURCE_H
#define TEXTURE_RESOURCE_H

#include <string_view>
#include <vulkan/vulkan.h>

namespace TANG
{
	class TextureResource
	{
	public:

		TextureResource();
		~TextureResource();
		TextureResource(const TextureResource& other);
		TextureResource(TextureResource&& other);
		TextureResource& operator=(const TextureResource& other);

		void Create(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, std::string_view fileName);
		void Destroy(VkDevice logicalDevice);

		void TransitionLayout(VkImageLayout destinationLayout);

	private:

		void CreateBaseImage(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkSampleCountFlagBits numSamples, VkFormat format,
			VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
		void CreateImageView(VkDevice logicalDevice, VkImageAspectFlags aspectFlags);
		void CreateSampler(VkDevice logicalDevice, float maxAnisotropy);

		void CopyFromBuffer(VkBuffer buffer);
		void GenerateMipmaps();

		// NOTE - This function does NOT clean up the allocated memory!!
		void ResetMembers();

		bool HasStencilComponent(VkFormat format);

		uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

	private:

		std::string name;
		uint32_t mipLevels;
		uint32_t width;
		uint32_t height;

		VkImage baseImage;
		VkDeviceMemory imageMemory;
		VkImageView imageView;
		VkSampler sampler;
		VkFormat format;
		VkImageLayout layout;

	};
}

#endif