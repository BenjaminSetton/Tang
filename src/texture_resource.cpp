
#include <cmath>

#include "utils/logger.h"
#include "utils/sanity_check.h"
#include "texture_resource.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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
		imageMemory(other.imageMemory), imageView(other.imageView), sampler(other.sampler), format(other.format)
	{
	}

	TextureResource::TextureResource(TextureResource&& other) : name(std::move(other.name)), width(std::move(other.width)),
		height(std::move(other.height)), mipLevels(std::move(other.mipLevels)), baseImage(std::move(other.baseImage)),
		imageMemory(std::move(other.imageMemory)), imageView(std::move(other.imageView)), sampler(std::move(other.sampler)),
		format(std::move(other.format))
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

		return *this;
	}

	void TextureResource::Create(std::string_view fileName, VkDevice logicalDevice)
	{
		int _width, _height, _channels;
		stbi_uc* pixels = stbi_load(fileName.data(), &_width, &_height, &_channels, STBI_rgb_alpha);
		if (pixels == nullptr)
		{
			TNG_ASSERT_MSG(false, "Failed to create texture from file '%s'!", fileName.data());
		}

		width = static_cast<uint32_t>(_width);
		height = static_cast<uint32_t>(_height);
		mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

		VkDeviceSize imageSize = width * height * 4;
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(logicalDevice, stagingBufferMemory);

		// Now that we've copied over the texture data to the staging buffer, we don't need the original pixels array anymore
		stbi_image_free(pixels);

		CreateBaseImage(VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyFromBuffer(stagingBuffer);
		GenerateMipmaps();

		vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
	}

	void TextureResource::Destroy(VkDevice logicalDevice)
	{
		vkDestroySampler(logicalDevice, sampler, nullptr);
		vkDestroyImageView(logicalDevice, imageView, nullptr);
		vkDestroyImage(logicalDevice, baseImage, nullptr);
		vkFreeMemory(logicalDevice, imageMemory, nullptr);

		ResetMembers();
	}

	void TextureResource::TransitionLayout(VkImageLayout destinationLayout)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(commandPools[TRANSFER_QUEUE]);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
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

		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
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

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
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
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		EndSingleTimeCommands(commandBuffer, TRANSFER_QUEUE);
	}

	void TextureResource::CreateBaseImage(VkDevice logicalDevice, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format,
		VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = numSamples;

		if (vkCreateImage(logicalDevice, &imageInfo, nullptr, &baseImage) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(logicalDevice, baseImage, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to allocate image memory!");
		}

		vkBindImageMemory(logicalDevice, baseImage, imageMemory, 0);
	}

	void TextureResource::CreateImageView(VkDevice logicalDevice, VkImageAspectFlags aspectFlags)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = baseImage;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = format;
		createInfo.subresourceRange.aspectMask = aspectFlags;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = mipLevels;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &imageView) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create texture image view!");
		}
	}

	void TextureResource::CreateSampler(VkDevice logicalDevice, float maxAnisotropy)
	{
		//VkPhysicalDeviceProperties properties{};
		//vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = maxAnisotropy;/*properties.limits.maxSamplerAnisotropy*/;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = static_cast<float>(mipLevels);

		if (vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create texture sampler!");
		}
	}

	void TextureResource::CopyFromBuffer()
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(commandPools[TRANSFER_QUEUE]);

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

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		EndSingleTimeCommands(commandBuffer, TRANSFER_QUEUE);
	}

	void TextureResource::GenerateMipmaps()
	{

	}

	void TextureResource::ResetMembers()
	{
		name = "";
		mipLevels = 0;
		width = 0;
		height = 0;
		baseImage = VK_NULL_HANDLE;
		imageMemory = VK_NULL_HANDLE;
		imageView = VK_NULL_HANDLE;
		sampler = VK_NULL_HANDLE;
		format = VK_FORMAT_UNDEFINED;
	}

	bool TextureResource::HasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	uint32_t TextureResource::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
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