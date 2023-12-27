
#include <cmath>

#include "cmd_buffer/disposable_command.h"
#include "command_pool_registry.h"
#include "data_buffer/staging_buffer.h"
#include "device_cache.h"
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

	TextureResource::TextureResource(TextureResource&& other) noexcept : name(std::move(other.name)), width(std::move(other.width)),
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

	void TextureResource::Create(const BaseImageCreateInfo* baseImageInfo, const ImageViewCreateInfo* viewInfo, const SamplerCreateInfo* samplerInfo)
	{
		if(baseImageInfo != nullptr) CreateBaseImage_Helper(baseImageInfo);
		if(viewInfo != nullptr)      CreateImageView(viewInfo);
		if(samplerInfo != nullptr)   CreateSampler(samplerInfo);
	}

	void TextureResource::CreateFromFile(std::string_view fileName, const BaseImageCreateInfo* createInfo, const ImageViewCreateInfo* viewInfo, const SamplerCreateInfo* samplerInfo)
	{
		CreateBaseImageFromFile(fileName, createInfo);
		if(viewInfo != nullptr) CreateImageView(viewInfo);
		if(samplerInfo != nullptr) CreateSampler(samplerInfo);
	}

	void TextureResource::Destroy()
	{
		VkDevice logicalDevice = GetLogicalDevice();

		if (sampler != VK_NULL_HANDLE) vkDestroySampler(logicalDevice, sampler, nullptr);

		DestroyImageView();

		if (baseImage != VK_NULL_HANDLE) vkDestroyImage(logicalDevice, baseImage, nullptr);
		if (imageMemory != VK_NULL_HANDLE) vkFreeMemory(logicalDevice, imageMemory, nullptr);

		ResetMembers();
	}

	void TextureResource::DestroyImageView()
	{
		if (imageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(GetLogicalDevice(), imageView, nullptr);
			imageView = VK_NULL_HANDLE;
		}
	}

	void TextureResource::TransitionLayout(VkImageLayout destinationLayout)
	{
		if (IsInvalid())
		{
			LogError("Attempting to transition layout of invalid texture!");
			return;
		}

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
		barrier.subresourceRange.layerCount = arrayLayers;
		barrier.srcAccessMask = 0; // TODO
		barrier.dstAccessMask = 0; // TODO

		VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_NONE;
		VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_NONE;
		QueueType commandQueueType = QueueType::TRANSFER;

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

		if (layout == VK_IMAGE_LAYOUT_UNDEFINED && destinationLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			commandQueueType = QueueType::TRANSFER;
		}
		else if (layout == VK_IMAGE_LAYOUT_UNDEFINED && destinationLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT; // Let's block on the vertex shader for now...

			commandQueueType = QueueType::GRAPHICS;
		}
		else if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && destinationLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // I guess this depends on whether we're using the texture in the vertex or pixel shader?

			commandQueueType = QueueType::GRAPHICS;
		}
		else if (layout == VK_IMAGE_LAYOUT_UNDEFINED && destinationLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

			commandQueueType = QueueType::GRAPHICS;
		}
		else
		{
			TNG_ASSERT_MSG(false, "Unsupported layout transition!");
		}

		{
			DisposableCommand command(commandQueueType);

			vkCmdPipelineBarrier(
				command.GetBuffer(),
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}

		// Success!
		layout = destinationLayout;
	}

	VkImageView TextureResource::GetImageView() const 
	{
		return imageView;
	}

	VkSampler TextureResource::GetSampler() const
	{
		return sampler;
	}

	VkFormat TextureResource::GetFormat() const
	{
		return format;
	}

	VkImageLayout TextureResource::GetLayout() const
	{
		return layout;
	}

	bool TextureResource::IsInvalid() const
	{
		return !isValid;
	}

	void TextureResource::CreateBaseImage(const BaseImageCreateInfo* baseImageInfo)
	{
		CreateBaseImage_Helper(baseImageInfo);
	}

	void TextureResource::CreateBaseImageFromFile(std::string_view filePath, const BaseImageCreateInfo* createInfo)
	{
		int _width, _height, _channels;
		void* data;
		if (stbi_is_hdr(filePath.data()))
		{
			data = stbi_loadf(filePath.data(), &_width, &_height, &_channels, STBI_rgb_alpha);
		}
		else
		{
			data = stbi_load(filePath.data(), &_width, &_height, &_channels, STBI_rgb_alpha);
		}

		if (data == nullptr)
		{
			LogError("Failed to create texture from file '%s'!", filePath.data());
			return;
		}

		// Get the fileName from the path
		name = filePath.substr(filePath.rfind("/") + 1, filePath.size());

		// Calculate amount of mipmaps
		uint32_t exactMips = CalculateMipLevelsFromSize(_width, _height);

		BaseImageCreateInfo baseImageInfo{};
		baseImageInfo.width = _width;
		baseImageInfo.height = _height;
		baseImageInfo.format = createInfo->format;
		baseImageInfo.usage = createInfo->usage;
		baseImageInfo.mipLevels = exactMips;
		baseImageInfo.samples = createInfo->samples;
		CreateBaseImage_Helper(&baseImageInfo);

		VkDeviceSize imageSize = _width * _height * bytesPerPixel;
		CopyDataIntoImage(data, imageSize);

		// Now that we've copied over the data to the texture image, we don't need the original pixels array anymore
		stbi_image_free(data);

		GenerateMipmaps(); // This sets the usage to SHADER_READ_ONLY after it's done
	}

	void TextureResource::CreateImageView(const ImageViewCreateInfo* viewInfo)
	{
		VkDevice logicalDevice = GetLogicalDevice();

		if (IsInvalid())
		{
			LogError("Attempting to create image view, but base image has not yet been created!");
			return;
		}

		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = baseImage;
		createInfo.viewType = viewInfo->viewType;
		createInfo.format = format;
		createInfo.subresourceRange.aspectMask = viewInfo->aspect;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = mipLevels;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (viewInfo->viewType == VK_IMAGE_VIEW_TYPE_CUBE)
		{
			createInfo.subresourceRange.layerCount = 6;
		}

		if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &imageView) != VK_SUCCESS)
		{
			LogError(false, "Failed to create texture image view!");
		}
	}

	void TextureResource::CreateSampler(const SamplerCreateInfo* samplerInfo)
	{
		VkDevice logicalDevice = GetLogicalDevice();

		VkSamplerCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.magFilter = samplerInfo->magnificationFilter;
		createInfo.minFilter = samplerInfo->minificationFilter;
		createInfo.addressModeU = samplerInfo->addressModeUVW;
		createInfo.addressModeV = samplerInfo->addressModeUVW;
		createInfo.addressModeW = samplerInfo->addressModeUVW;
		createInfo.anisotropyEnable = VK_TRUE;
		createInfo.maxAnisotropy = samplerInfo->maxAnisotropy;
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

	void TextureResource::CreateBaseImage_Helper(const BaseImageCreateInfo* baseImageInfo)
	{
		VkDevice logicalDevice = GetLogicalDevice();


		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = baseImageInfo->width;
		imageInfo.extent.height = baseImageInfo->height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = baseImageInfo->mipLevels;
		imageInfo.arrayLayers = baseImageInfo->arrayLayers;
		imageInfo.format = baseImageInfo->format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = baseImageInfo->usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = baseImageInfo->samples;
		imageInfo.flags = baseImageInfo->flags;

		// Calculate mip count if baseImageInfo->mipLevels is equal to 0. The Vulkan spec disallows this, so this is a valid alternative
		if (imageInfo.mipLevels == 0)
		{
			uint32_t calculatedMips = CalculateMipLevelsFromSize(imageInfo.extent.width, imageInfo.extent.height);
			LogInfo("Texture resource specified an invalid 0 mip levels for a %ux%u, using %u mip levels instead", imageInfo.extent.width, imageInfo.extent.height, calculatedMips);
			
			imageInfo.mipLevels = calculatedMips;
		}

		// Create the image
		if (vkCreateImage(logicalDevice, &imageInfo, nullptr, &baseImage) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create image!");
		}

		// Cache some of the image data
		width = baseImageInfo->width;
		height = baseImageInfo->height;
		mipLevels = baseImageInfo->mipLevels;
		format = baseImageInfo->format;
		bytesPerPixel = GetBytesPerPixelFromFormat(baseImageInfo->format);
		arrayLayers = baseImageInfo->arrayLayers;

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(logicalDevice, baseImage, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to allocate image memory!");
		}

		vkBindImageMemory(logicalDevice, baseImage, imageMemory, 0);

		isValid = true;
	}

	void TextureResource::CreateImageViewFromBase(VkImage _baseImage, VkFormat _format, uint32_t _mipLevels, VkImageAspectFlags _aspect)
	{
		VkDevice logicalDevice = GetLogicalDevice();

		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = _baseImage;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // NOTE - If we allow cubemaps here, we should also change layerCount to 6
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

	void TextureResource::CopyDataIntoImage(void* data, VkDeviceSize bytes)
	{
		if (IsInvalid())
		{
			LogError("Attempting to copy data into texture, but texture has not been created!");
			return;
		}

		if (bytes == 0)
		{
			LogWarning("Attempting to copy data into texture image, but a size of 0 bytes was specified!");
			return;
		}

		// If bytes is greater than the image size, then allocate up until the image size (and produce a warning)
		VkDeviceSize imageSize = width * height * bytesPerPixel;
		if (bytes > imageSize)
		{
			LogWarning("Attempting to copy %u bytes into texture image, when the image size is only %u. Only %u bytes will be copied.", bytes, imageSize, imageSize);
		}
		
		VkDeviceSize actualSize = std::min(bytes, imageSize);

		StagingBuffer stagingBuffer;
		stagingBuffer.Create(actualSize);
		stagingBuffer.CopyIntoBuffer(data, actualSize);

		VkImageLayout oldLayout = layout;

		TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyFromBuffer(stagingBuffer.GetBuffer());
		if (oldLayout != VK_IMAGE_LAYOUT_UNDEFINED)
		{
			TransitionLayout(oldLayout);
		}

		stagingBuffer.Destroy();
	}

	void TextureResource::CopyFromBuffer(VkBuffer buffer)
	{
		if (IsInvalid())
		{
			LogError("Attempting to copy from buffer, but base image has not yet been created!");
			return;
		}

		DisposableCommand command(QueueType::TRANSFER);

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

	void TextureResource::GenerateMipmaps()
	{
		VkPhysicalDevice physicalDevice = GetPhysicalDevice();

		if (IsInvalid())
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

		DisposableCommand command(QueueType::GRAPHICS);

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

		// Success!
		layout = barrier.newLayout;
	}

	void TextureResource::ResetMembers()
	{
		LogInfo("Resetting texture resource members. Ensure memory was cleaned up, otherwise this is a leak");

		name = "";
		mipLevels = 0;
		width = 0;
		height = 0;
		bytesPerPixel = 0;
		arrayLayers = 1;
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

	uint32_t TextureResource::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties = DeviceCache::Get().GetPhysicalDeviceMemoryProperties();

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

	uint32_t TextureResource::GetBytesPerPixelFromFormat(VkFormat texFormat)
	{
		switch (texFormat)
		{
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_B8G8R8A8_SRGB:
		case VK_FORMAT_D32_SFLOAT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		{
			return 4;
		}
		case VK_FORMAT_R16G16B16A16_SFLOAT:
		{
			return 8;
		}
		case VK_FORMAT_R32G32B32A32_SFLOAT:
		{
			// HDR
			return 16;
		}
		}

		TNG_ASSERT_MSG(false, "Attempting to get bytes per pixel from format, but texture format is not yet supported!");
		return 0;
	}

	uint32_t TextureResource::CalculateMipLevelsFromSize(uint32_t _width, uint32_t _height)
	{
		double dWidth = static_cast<double>(_width);
		double dHeight = static_cast<double>(_height);
		double exactMips = log2(std::min(dWidth, dHeight));
		if ((exactMips - std::trunc(exactMips)) != 0)
		{
			LogWarning("Texture '%s' has dimensions which are not a power-of-two (%u, %u)! This will generate inefficient mip-maps", name.c_str(), _width, _height);
		}

		return static_cast<uint32_t>(exactMips);
	}
}