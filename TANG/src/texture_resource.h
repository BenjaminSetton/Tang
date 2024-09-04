#ifndef TEXTURE_RESOURCE_H
#define TEXTURE_RESOURCE_H

#include <optional>
#include <string_view>
#include <vector>
#include <vulkan/vulkan.h>

#include "queue_types.h"

namespace TANG
{
	enum class ImageViewScope
	{
		ENTIRE_IMAGE,		// Generates an image view for the entire image, including all mip levels and cubemap faces (in cases where image is a cubemap)
		PER_MIP_LEVEL		// Generates an image view for every mip level
	};

	// Holds all the information necessary to create an image view for a TextureResource object.
	// This is similar to Vulkan's VkImageViewCreateInfo struct, but this separate struct exists
	// for a few reasons:
	// 1) Prevents the caller from setting/changing unsupported options
	// 2) Saves the caller from filling out redundant fields, such as the base image or structure type in this case
	struct ImageViewCreateInfo
	{
		VkImageAspectFlags aspect;
		VkImageViewType viewType		= VK_IMAGE_VIEW_TYPE_2D;
		ImageViewScope viewScope		= ImageViewScope::ENTIRE_IMAGE;
	};

	// Holds all the information necessary to create a sampler for a TextureResource object.
	// This exists for the same reasons listed above.
	struct SamplerCreateInfo
	{
		VkFilter minificationFilter				= VK_FILTER_LINEAR;
		VkFilter magnificationFilter			= VK_FILTER_LINEAR;
		VkSamplerMipmapMode mipmapMode			= VK_SAMPLER_MIPMAP_MODE_LINEAR;
		VkSamplerAddressMode addressModeUVW		= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		bool enableAnisotropicFiltering			= true;
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
		bool generateMipMaps			= true;
	};

	// Forward declarations
	class CommandBuffer;
	class DisposableCommand;

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
		void CopyFromData(void* data, VkDeviceSize bytes);

		// Copies the image data from the provided source texture, including all the specified mips
		void CopyFromTexture(CommandBuffer* cmdBuffer, TextureResource* sourceTexture, uint32_t baseMip, uint32_t mipCount);

		// Deletes the existing image views (if any) and creates them depending on the data contained within the viewInfo parameter
		void RecreateImageViews(const ImageViewCreateInfo* viewInfo);

		void Destroy();
		void DestroyBaseImage();
		void DestroyImageViews();

		void TransitionLayout(CommandBuffer* commandBuffer, VkImageLayout sourceLayout, VkImageLayout destinationLayout);
		void TransitionLayout_Immediate(VkImageLayout sourceLayout, VkImageLayout destinationLayout);
		void TransitionLayout_Force(VkImageLayout destinationLayout); // This function must only be used to reflect implicit layout transitions which happen after the render pass ends. It does not introduce a pipeline barrier like the other TransitionLayout() functions

		void InsertPipelineBarrier(const CommandBuffer* cmdBuffer,
			VkAccessFlags srcAccessFlags,
			VkAccessFlags dstAccessFlags,
			VkPipelineStageFlags srcStage,
			VkPipelineStageFlags dstStage,
			uint32_t baseMip,
			uint32_t mipCount);

		void GenerateMipmaps(CommandBuffer* cmdBuffer, uint32_t mipCount);

		VkImageView GetImageView(uint32_t viewIndex) const;
		VkSampler GetSampler() const;
		VkFormat GetFormat() const;
		VkImageLayout GetLayout() const;
		bool IsInvalid() const;

		bool IsDepthTexture() const;
		bool HasStencilComponent() const;

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;

		uint32_t GetAllocatedMipLevels() const;
		uint32_t GetGeneratedMipLevels() const;
		uint32_t CalculateMipLevelsFromSize() const;

		ImageViewScope GetViewScope() const;

	private:

		void CreateBaseImage(const BaseImageCreateInfo* baseImageInfo);
		void CreateBaseImageFromFile(std::string_view filePath, const BaseImageCreateInfo* createInfo);

		// NOTE - This function stalls the graphics queue twice!
		void GenerateMipmaps_Immediate(uint32_t mipCount);
		void GenerateMipmaps_Helper(VkCommandBuffer cmdBuffer, uint32_t mipCount);

		// Create image view from a previously-created base image (through CreateBaseImage or CreateBaseImageFromFile)
		void CreateImageViews(const ImageViewCreateInfo* viewInfo);

		void CreateSampler(const SamplerCreateInfo* samplerInfo);

		void CreateBaseImage_Helper(const BaseImageCreateInfo* baseImageInfo);

		void CopyFromBuffer(VkBuffer buffer, uint32_t destinationMipLevel);

		void TransitionLayout_Internal(VkCommandBuffer commandBuffer, 
			TextureResource* baseTexture, 
			VkImageLayout sourceLayout, 
			VkImageLayout destinationLayout);

		std::optional<VkImageMemoryBarrier> TransitionLayout_Helper(const TextureResource* baseTexture,
			VkImageLayout sourceLayout,
			VkImageLayout destinationLayout,
			VkPipelineStageFlags& out_sourceStage,
			VkPipelineStageFlags& out_destinationStage,
			QueueType& out_queueType);

		void InsertPipelineBarrier_Helper(VkCommandBuffer cmdBuffer,
			VkAccessFlags srcAccessFlags,
			VkAccessFlags dstAccessFlags,
			VkPipelineStageFlags srcStage,
			VkPipelineStageFlags dstStage,
			uint32_t baseMip,
			uint32_t mipCount);

		void InsertPipelineBarrier_Internal(VkCommandBuffer cmdBuffer, 
			VkPipelineStageFlags srcStage, 
			VkPipelineStageFlags dstStage, 
			VkImageMemoryBarrier barrier);

		// NOTE - This function does NOT clean up the allocated memory!!
		void ResetMembers();

		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		uint32_t GetBytesPerPixelFromFormat(VkFormat format);

		uint32_t CalculateMipLevelsFromSize(uint32_t width, uint32_t height) const;

	private:

		std::string name;
		bool isValid;
		uint32_t bytesPerPixel;
		VkImageLayout layout;
		uint32_t generatedMips; // Defines the number of mipmaps that have been generated through GenerateMipmaps()

		BaseImageCreateInfo baseImageInfo;
		ImageViewCreateInfo imageViewInfo;
		SamplerCreateInfo samplerInfo;

		VkImage baseImage;
		VkDeviceMemory imageMemory;
		std::vector<VkImageView> imageViews;
		VkSampler sampler;


	};
}

#endif