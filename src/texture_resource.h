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
		VkImageViewType viewType		= VK_IMAGE_VIEW_TYPE_2D;
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
		uint32_t arrayLayers			= 1;
		VkImageCreateFlags flags		= 0;
	};

	class TextureResource
	{
	public:

		TextureResource();
		~TextureResource();
		TextureResource(const TextureResource& other);
		TextureResource(TextureResource&& other) noexcept;
		TextureResource& operator=(const TextureResource& other);

		void Create(const BaseImageCreateInfo* baseImageInfo, const ImageViewCreateInfo* viewInfo = nullptr, const SamplerCreateInfo* samplerInfo = nullptr);
		
		// NOTE - The width, height and mipmaps field from BaseImageCreateInfo in unused in this function. Those get pulled from the loaded image directly
		void CreateFromFile(std::string_view fileName, const BaseImageCreateInfo* createInfo, const ImageViewCreateInfo* viewInfo = nullptr, const SamplerCreateInfo* samplerInfo = nullptr);

		// Create image view from a provided base image. This is used to create an image into the swapchain's provided base images, since
		// we don't want to create our own base images in this case
		void CreateImageViewFromBase(VkImage baseImage, VkFormat format, uint32_t mipLevels, VkImageAspectFlags aspect);

		// Copies an arbitrary amount of data into the texture image buffer, up to the maximum size declared when creating the texture
		// NOTE - The usage of the texture will remain the same, EXCEPT if it has an UNDEFINED usage. In that case the usage will become
		//        TRANSFER_DST_OPTIMAL
		void CopyDataIntoImage(void* data, VkDeviceSize bytes);

		void Destroy();
		void DestroyImageView();

		void TransitionLayout(VkImageLayout destinationLayout);

		VkImageView GetImageView() const;
		VkSampler GetSampler() const;
		VkFormat GetFormat() const;
		VkImageLayout GetLayout() const;
		bool IsInvalid() const;

	private:

		void CreateBaseImage(const BaseImageCreateInfo* baseImageInfo);
		void CreateBaseImageFromFile(std::string_view filePath, const BaseImageCreateInfo* createInfo);

		// Create image view from a previously-created base image (through CreateBaseImage or CreateBaseImageFromFile)
		void CreateImageView(const ImageViewCreateInfo* viewInfo);

		void CreateSampler(const SamplerCreateInfo* samplerInfo);

		void CreateBaseImage_Helper(const BaseImageCreateInfo* baseImageInfo);

		void CopyFromBuffer(VkBuffer buffer);
		void GenerateMipmaps();

		// NOTE - This function does NOT clean up the allocated memory!!
		void ResetMembers();

		bool HasStencilComponent(VkFormat format);

		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		uint32_t GetBytesPerPixelFromFormat(VkFormat format);

		uint32_t CalculateMipLevelsFromSize(uint32_t width, uint32_t height);

	private:

		std::string name;
		uint32_t mipLevels;
		uint32_t width;
		uint32_t height;
		uint32_t bytesPerPixel;
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