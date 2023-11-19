#ifndef TEXTURE_RESOURCE_H
#define TEXTURE_RESOURCE_H

#include <string_view>
#include <vulkan/vulkan.h>

namespace TANG
{
	// Holds all the information necessary to create an image view for a TextureResource object.
	// This is similar to Vulkan's VkImageViewCreateInfo struct, but this separate struct exists
	// for a few reasons:
	// 1) Prevents the caller from setting/changing unsupported options
	// 2) Saves the caller from filling out redundant fields, such as the base image or structure type in this case
	struct ImageViewCreateInfo
	{
		VkImageAspectFlags aspect;
	};

	// Holds all the information necessary to create a sampler for a TextureResource object.
	// This exists for the same reasons listed above.
	struct SamplerCreateInfo
	{
		VkFilter minificationFilter				= VK_FILTER_LINEAR;
		VkFilter magnificationFilter			= VK_FILTER_LINEAR;
		VkSamplerAddressMode addressModeUVW		= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		float maxAnisotropy						= 1.0f;
	};

	struct BaseImageCreateInfo
	{
		uint32_t width					= 0;
		uint32_t height					= 0;
		VkFormat format					= VK_FORMAT_R8G8B8A8_SRGB;
		VkImageUsageFlags usage			= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		uint32_t mipLevels				= 1;
		VkSampleCountFlagBits samples	= VK_SAMPLE_COUNT_1_BIT;
	};

	class TextureResource
	{
	public:

		TextureResource();
		~TextureResource();
		TextureResource(const TextureResource& other);
		TextureResource(TextureResource&& other);
		TextureResource& operator=(const TextureResource& other);

		void Create(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, const BaseImageCreateInfo* baseImageInfo, const ImageViewCreateInfo* viewInfo = nullptr, const SamplerCreateInfo* samplerInfo = nullptr);
		void CreateFromFile(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, std::string_view fileName, const ImageViewCreateInfo* viewInfo = nullptr, const SamplerCreateInfo* samplerInfo = nullptr);

		// Create image view from a provided base image. This is used to create an image into the swapchain's provided base images, since
		// we don't want to create our own base images in this case
		void CreateImageViewFromBase(VkDevice logicalDevice, VkImage baseImage, VkFormat format, uint32_t mipLevels, VkImageAspectFlags aspect);

		void Destroy(VkDevice logicalDevice);
		void DestroyImageView(VkDevice logicalDevice);

		void TransitionLayout(VkDevice logicalDevice, VkImageLayout destinationLayout);

		VkImageView GetImageView() const;
		VkSampler GetSampler() const;
		VkFormat GetFormat() const;
		VkImageLayout GetLayout() const;
		bool IsValid() const;

	private:

		void CreateBaseImage(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, const BaseImageCreateInfo* baseImageInfo);
		void CreateBaseImageFromFile(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, std::string_view fileName);

		// Create image view from a previously-created base image (through CreateBaseImage or CreateBaseImageFromFile)
		void CreateImageView(VkDevice logicalDevice, const ImageViewCreateInfo* viewInfo);

		void CreateSampler(VkDevice logicalDevice, const SamplerCreateInfo* samplerInfo);

		void CreateBaseImage_Helper(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, const BaseImageCreateInfo* baseImageInfo);

		void CopyFromBuffer(VkDevice logicalDevice, VkBuffer buffer);
		void GenerateMipmaps(VkPhysicalDevice physicalDevice, VkDevice logicalDevice);

		// NOTE - This function does NOT clean up the allocated memory!!
		void ResetMembers();

		bool HasStencilComponent(VkFormat format);

		uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

	private:

		std::string name;
		uint32_t mipLevels;
		uint32_t width;
		uint32_t height;
		bool isValid;

		VkImage baseImage;
		VkDeviceMemory imageMemory;
		VkImageView imageView;
		VkSampler sampler;
		VkFormat format;
		VkImageLayout layout;

	};
}

#endif