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
		VkFilter minificationFilter, magnificationFilter;
		VkSamplerAddressMode addressModeUVW;
		float maxAnisotropy;
	};

	class TextureResource
	{
	public:

		TextureResource();
		~TextureResource();
		TextureResource(const TextureResource& other);
		TextureResource(TextureResource&& other);
		TextureResource& operator=(const TextureResource& other);

		void CreateBaseImage(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkFormat format, VkImageUsageFlags usage);
		void CreateImageView(VkDevice logicalDevice, const ImageViewCreateInfo& viewInfo);
		void CreateSampler(VkDevice logicalDevice, const SamplerCreateInfo& samplerInfo);

		void Destroy(VkDevice logicalDevice);

		void LoadFromFile(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, std::string_view fileName);
		void TransitionLayout(VkDevice logicalDevice, VkImageLayout destinationLayout);

		VkImageView GetImageView() const;
		bool IsValid() const;

		// Do not use unless it's a very specific case. This is used pretty much only for the swapchain images!
		void SetBaseImage(VkImage image);

	private:

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