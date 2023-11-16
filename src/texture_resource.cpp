
#include <cmath>

#include "cmd_buffer/disposable_command.h"
#include "command_pool_registry.h"
#include "data_buffer/staging_buffer.h"
#include "texture_resource.h"
#include "utils/logger.h"
#include "utils/sanity_check.h"

// Silence stb_image warnings:
// warning C4244: 'argument': conversion from 'int' to 'short', possible loss of data
#pragma warning(push)
#pragma warning(disable : 4244)

//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#pragma warning(pop)

namespace TANG
{

	TextureResource::TextureResource()
	{
		ResetMembers();
	}

	TextureResource::~TextureResource()
	{
		if (imageMemory != VK_NULL_HANDLE)
		{
			LogWarning("Texture '%s' has not been cleaned up, but destructor has been called!", name.c_str());
		}
	}

	TextureResource::TextureResource(const TextureResource& other) : name(other.name), width(other.width),
		height(other.height), mipLevels(other.mipLevels), baseImage(other.baseImage),
		imageMemory(other.imageMemory), imageView(other.imageView), sampler(other.sampler), format(other.format),
		layout(other.layout)
	{
		// TODO - Deep copy??
		LogInfo("Texture resource shallow-copied (copy-constructor)");
	}

	TextureResource::TextureResource(TextureResource&& other) : name(std::move(other.name)), width(std::move(other.width)),
		height(std::move(other.height)), mipLevels(std::move(other.mipLevels)), baseImage(std::move(other.baseImage)),
		imageMemory(std::move(other.imageMemory)), imageView(std::move(other.imageView)), sampler(std::move(other.sampler)),
		format(std::move(other.format)), layout(std::move(other.layout))
	{
		other.ResetMembers();
	}

	TextureResource& TextureResource::operator=(const TextureResource& other)
	{
		if (this == &other)
		{
			return *this;
		}

		name = other.name;
		width = other.width;
		height = other.height;
		mipLevels = other.mipLevels;
		baseImage = other.baseImage;
		imageMemory = other.imageMemory;
		imageView = other.imageView;
		sampler = other.sampler;
		format = other.format;
		layout = other.layout;

		// TODO - Deep copy??
		LogInfo("Texture resource shallow-copied (copy-assignment)");

		return *this;
	}

	void TextureResource::CreateBaseImage(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, const BaseImageCreateInfo& baseImageInfo)
	{
		CreateBaseImage_Helper(physicalDevice, logicalDevice, baseImageInfo);
	}

	void TextureResource::CreateBaseImageFromFile(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, std::string_view fileName)
	{
		int _width, _height, _channels;
		stbi_uc* pixels = stbi_load(fileName.data(), &_width, &_height, &_channels, STBI_rgb_alpha);
		if (pixels == nullptr)
		{
			LogError("Failed to create texture from file '%s'!", fileName.data());
		}

		//width = static_cast<uint32_t>(_width);
		//height = static_cast<uint32_t>(_height);

		VkDeviceSize imageSize = _width * _height * 4;
		/*VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);*/
		StagingBuffer stagingBuffer;
		stagingBuffer.Create(physicalDevice, logicalDevice, imageSize);

		//void* data;
		//vkMapMemory(logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
		//memcpy(data, pixels, static_cast<size_t>(imageSize));
		//vkUnmapMemory(logicalDevice, stagingBufferMemory);
		stagingBuffer.CopyIntoBuffer(logicalDevice, pixels, imageSize);

		// Now that we've copied over the texture data to the staging buffer, we don't need the original pixels array anymore
		stbi_image_free(pixels);

		// Calculate amount of mipmaps
		double exactMips = log2(std::min(_width, _height));
		if ((exactMips - std::trunc(exactMips)) != 0)
		{
			LogWarning("Texture '%s' has dimensions which are not a power-of-two (%u, %u)! This will generate inefficient mip-maps", fileName.data(), _width, _height);
		}
		/*mipLevels = static_cast<uint32_t>(exactMips);*/

		BaseImageCreateInfo baseImageInfo{};
		baseImageInfo.width = _width;
		baseImageInfo.height = _height;
		baseImageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		baseImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		baseImageInfo.mipLevels = static_cast<uint32_t>(exactMips);
		baseImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		CreateBaseImage_Helper(physicalDevice, logicalDevice, baseImageInfo);

		TransitionLayout(logicalDevice, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyFromBuffer(logicalDevice, stagingBuffer.GetBuffer());
		GenerateMipmaps(physicalDevice, logicalDevice);

		/*vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);*/
		stagingBuffer.Destroy(logicalDevice);
	}

	void TextureResource::DestroyAll(VkDevice logicalDevice)
	{
		if (sampler != VK_NULL_HANDLE) vkDestroySampler(logicalDevice, sampler, nullptr);
		if (imageView != VK_NULL_HANDLE) vkDestroyImageView(logicalDevice, imageView, nullptr);
		if (baseImage != VK_NULL_HANDLE) vkDestroyImage(logicalDevice, baseImage, nullptr);
		if (imageMemory != VK_NULL_HANDLE) vkFreeMemory(logicalDevice, imageMemory, nullptr);

		ResetMembers();
	}

	void TextureResource::DestroyImageView(VkDevice logicalDevice)
	{
		if (imageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(logicalDevice, imageView, nullptr);
			imageView = VK_NULL_HANDLE;
		}
	}

	void TextureResource::TransitionLayout(VkDevice logicalDevice, VkImageLayout destinationLayout)
	{
		if (!IsValid())
		{
			LogError("Attempting to transition layout of invalid texture!");
			return;
		}

		DisposableCommand command(logicalDevice, QueueType::TRANSFER);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = layout;
		barrier.newLayout = destinationLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = baseImage;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = 0; // TODO
		barrier.dstAccessMask = 0; // TODO

		VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_NONE;
		VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_NONE;

		if (destinationLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (HasStencilComponent(format))
			{
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		if (layout == VK_IMAGE_LAYOUT_UNDEFINED && destinationLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && destinationLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (layout == VK_IMAGE_LAYOUT_UNDEFINED && destinationLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else
		{
			TNG_ASSERT_MSG(false, "Unsupported layout transition!");
		}

		vkCmdPipelineBarrier(
			command.GetBuffer(),
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}

	VkImageView TextureResource::GetImageView() const
	{
		return imageView;
	}

	bool TextureResource::IsValid() const
	{
		return isValid;
	}

	void TextureResource::CreateBaseImage_Helper(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, const BaseImageCreateInfo& baseImageInfo)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = baseImageInfo.width;
		imageInfo.extent.height = baseImageInfo.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = baseImageInfo.mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.format = baseImageInfo.format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = baseImageInfo.usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = baseImageInfo.samples;

		if (vkCreateImage(logicalDevice, &imageInfo, nullptr, &baseImage) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(logicalDevice, baseImage, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to allocate image memory!");
		}

		vkBindImageMemory(logicalDevice, baseImage, imageMemory, 0);

		width = baseImageInfo.width;
		height = baseImageInfo.height;
		mipLevels = baseImageInfo.mipLevels;
		format = baseImageInfo.format;
		isValid = true;
	}

	void TextureResource::CreateImageView(VkDevice logicalDevice, const ImageViewCreateInfo& viewInfo)
	{
		if (!IsValid())
		{
			LogError("Attempting to create image view, but base image has not yet been created!");
			return;
		}

		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = baseImage;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = format;
		createInfo.subresourceRange.aspectMask = viewInfo.aspect;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = mipLevels;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &imageView) != VK_SUCCESS)
		{
			LogError(false, "Failed to create texture image view!");
		}
	}

	void TextureResource::CreateImageViewFromBase(VkDevice logicalDevice, VkImage _baseImage, VkFormat _format, uint32_t _mipLevels, VkImageAspectFlags _aspect)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = _baseImage;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = _format;
		createInfo.subresourceRange.aspectMask = _aspect;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = _mipLevels;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &imageView) != VK_SUCCESS)
		{
			LogError(false, "Failed to create texture image view!");
		}
	}

	void TextureResource::CreateSampler(VkDevice logicalDevice, const SamplerCreateInfo& samplerInfo)
	{
		VkSamplerCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.magFilter = samplerInfo.magnificationFilter;
		createInfo.minFilter = samplerInfo.minificationFilter;
		createInfo.addressModeU = samplerInfo.addressModeUVW;
		createInfo.addressModeV = samplerInfo.addressModeUVW;
		createInfo.addressModeW = samplerInfo.addressModeUVW;
		createInfo.anisotropyEnable = VK_TRUE;
		createInfo.maxAnisotropy = samplerInfo.maxAnisotropy/*properties.limits.maxSamplerAnisotropy*/;
		createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		createInfo.unnormalizedCoordinates = VK_FALSE;
		createInfo.compareEnable = VK_FALSE;
		createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		createInfo.mipLodBias = 0.0f;
		createInfo.minLod = 0.0f;
		createInfo.maxLod = static_cast<float>(mipLevels);

		if (vkCreateSampler(logicalDevice, &createInfo, nullptr, &sampler) != VK_SUCCESS)
		{
			LogError(false, "Failed to create texture sampler!");
		}
	}

	void TextureResource::CopyFromBuffer(VkDevice logicalDevice, VkBuffer buffer)
	{
		if (!IsValid())
		{
			LogError("Attempting to copy from buffer, but base image has not yet been created!");
			return;
		}

		DisposableCommand command(logicalDevice, QueueType::TRANSFER);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(command.GetBuffer(), buffer, baseImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}

	void TextureResource::GenerateMipmaps(VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
	{
		if (!IsValid())
		{
			LogError("Attempting to generate mipmaps but base image has not yet been created!");
			return;
		}

		// Check if the texture format we want to use supports linear blitting
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		{
			TNG_ASSERT_MSG(false, "Texture image does not support linear blitting!");
		}

		DisposableCommand command(logicalDevice, QueueType::GRAPHICS);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = baseImage;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = width;
		int32_t mipHeight = height;

		for (uint32_t i = 1; i < mipLevels; i++)
		{

			// Transition image from transfer dst optimal to transfer src optimal, since we're reading from this image
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(command.GetBuffer(),
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(command.GetBuffer(),
				baseImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				baseImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			// Transition image from src transfer optimal to shader read only
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(command.GetBuffer(),
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		// Transfer the last mip level to shader read-only because this wasn't handled by the loop above
		// (since we didn't blit from the last image)
		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(command.GetBuffer(),
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);
	}

	void TextureResource::ResetMembers()
	{
		name = "";
		mipLevels = 0;
		width = 0;
		height = 0;
		isValid = false;
		baseImage = VK_NULL_HANDLE;
		imageMemory = VK_NULL_HANDLE;
		imageView = VK_NULL_HANDLE;
		sampler = VK_NULL_HANDLE;
		format = VK_FORMAT_UNDEFINED;
		layout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	bool TextureResource::HasStencilComponent(VkFormat _format)
	{
		return _format == VK_FORMAT_D32_SFLOAT_S8_UINT || _format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	uint32_t TextureResource::FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		TNG_ASSERT_MSG(false, "Failed to find suitable memory type!");
		return std::numeric_limits<uint32_t>::max();
	}

}