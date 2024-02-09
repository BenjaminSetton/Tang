
#include <cmath>
#include <optional>

#include "cmd_buffer/disposable_command.h"
#include "cmd_buffer/command_buffer.h"
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

	TextureResource::TextureResource(const TextureResource& other) : name(other.name), isValid(other.isValid), bytesPerPixel(other.bytesPerPixel),
		layout(other.layout), generatedMips(other.generatedMips), baseImageInfo(other.baseImageInfo), imageViewInfo(other.imageViewInfo), samplerInfo(other.samplerInfo), 
		baseImage(other.baseImage), imageMemory(other.imageMemory), imageViews(other.imageViews), sampler(other.sampler)
	{
		// TODO - Deep copy??
		LogInfo("Texture resource shallow-copied (copy-constructor)");
	}

	TextureResource::TextureResource(TextureResource&& other) noexcept : name(std::move(other.name)), isValid(std::move(other.isValid)), bytesPerPixel(std::move(other.bytesPerPixel)),
		layout(std::move(other.layout)), generatedMips(std::move(other.generatedMips)), baseImageInfo(std::move(other.baseImageInfo)), imageViewInfo(std::move(other.imageViewInfo)), samplerInfo(std::move(other.samplerInfo)),
		baseImage(std::move(other.baseImage)), imageMemory(std::move(other.imageMemory)), imageViews(std::move(other.imageViews)), sampler(std::move(other.sampler))
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
		isValid = other.isValid;
		bytesPerPixel = other.bytesPerPixel;
		layout = other.layout;
		generatedMips = other.generatedMips;

		baseImageInfo = other.baseImageInfo;
		imageViewInfo = other.imageViewInfo;
		samplerInfo = other.samplerInfo;

		baseImage = other.baseImage;
		imageMemory = other.imageMemory;
		imageViews = other.imageViews;
		sampler = other.sampler;

		// TODO - Deep copy??
		LogInfo("Texture resource shallow-copied (copy-assignment)");

		return *this;
	}

	void TextureResource::Create(const BaseImageCreateInfo* _baseImageInfo, const ImageViewCreateInfo* _viewInfo, const SamplerCreateInfo* _samplerInfo)
	{
		if(_baseImageInfo != nullptr) CreateBaseImage_Helper(_baseImageInfo);
		if(_viewInfo != nullptr)      CreateImageViews(_viewInfo);
		if(_samplerInfo != nullptr)   CreateSampler(_samplerInfo);
	}

	void TextureResource::CreateFromFile(std::string_view fileName, const BaseImageCreateInfo* createInfo, const ImageViewCreateInfo* viewInfo, const SamplerCreateInfo* _samplerInfo)
	{
		CreateBaseImageFromFile(fileName, createInfo);
		if(viewInfo != nullptr) CreateImageViews(viewInfo);
		if(_samplerInfo != nullptr) CreateSampler(_samplerInfo);
	}

	void TextureResource::Destroy()
	{
		VkDevice logicalDevice = GetLogicalDevice();

		if (sampler != VK_NULL_HANDLE) vkDestroySampler(logicalDevice, sampler, nullptr);

		DestroyImageViews();
		DestroyBaseImage();

		ResetMembers();
	}

	void TextureResource::DestroyBaseImage()
	{
		VkDevice logicalDevice = GetLogicalDevice();

		if (baseImage != VK_NULL_HANDLE)
		{
			vkDestroyImage(logicalDevice, baseImage, nullptr);
			baseImage = VK_NULL_HANDLE;
		}

		if (imageMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(logicalDevice, imageMemory, nullptr);
			imageMemory = VK_NULL_HANDLE;
		}
	}

	void TextureResource::DestroyImageViews()
	{
		auto logicalDevice = GetLogicalDevice();
		for (auto& imageView : imageViews)
		{
			if (imageView != VK_NULL_HANDLE)
			{
				vkDestroyImageView(logicalDevice, imageView, nullptr);
				imageView = VK_NULL_HANDLE;
			}
		}

		imageViews.clear();
	}

	void TextureResource::TransitionLayout(CommandBuffer* commandBuffer, VkImageLayout sourceLayout, VkImageLayout destinationLayout)
	{
		if (commandBuffer)
		{
			TransitionLayout_Internal(commandBuffer->GetBuffer(), this, sourceLayout, destinationLayout);
		}
	}

	void TextureResource::TransitionLayout_Immediate(VkImageLayout sourceLayout, VkImageLayout destinationLayout)
	{
		DisposableCommand command(QueueType::GRAPHICS, true);
		TransitionLayout_Internal(command.GetBuffer(), this, sourceLayout, destinationLayout);
	}

	void TextureResource::TransitionLayout_Force(VkImageLayout destinationLayout)
	{
		layout = destinationLayout;
	}

	void TextureResource::InsertPipelineBarrier(const CommandBuffer* cmdBuffer,
		VkAccessFlags srcAccessFlags,
		VkAccessFlags dstAccessFlags,
		VkPipelineStageFlags srcStage,
		VkPipelineStageFlags dstStage,
		uint32_t baseMip,
		uint32_t mipCount)
	{
		if (cmdBuffer == nullptr)
		{
			LogError("Attempting to insert pipeline barrier, but no command buffer was provided!");
			return;
		}

		if (mipCount == 0)
		{
			mipCount = baseImageInfo.mipLevels - baseMip;
		}

		InsertPipelineBarrier_Helper(cmdBuffer->GetBuffer(), srcAccessFlags, dstAccessFlags, srcStage, dstStage, baseMip, mipCount);
	}

	void TextureResource::GenerateMipmaps(CommandBuffer* cmdBuffer, uint32_t mipCount)
	{
		GenerateMipmaps_Helper(cmdBuffer->GetBuffer(), mipCount);
	}

	void TextureResource::GenerateMipmaps_Immediate(uint32_t mipCount)
	{
		DisposableCommand cmdBuffer(QueueType::GRAPHICS, true);
		GenerateMipmaps_Helper(cmdBuffer.GetBuffer(), mipCount);
	}

	VkImageView TextureResource::GetImageView(uint32_t viewIndex) const 
	{
		if (viewIndex < 0 || viewIndex >= imageViews.size())
		{
			LogWarning("Image view index out of range!");
			return VK_NULL_HANDLE;
		}

		return imageViews.at(viewIndex);
	}

	VkSampler TextureResource::GetSampler() const
	{
		return sampler;
	}

	VkFormat TextureResource::GetFormat() const
	{
		return baseImageInfo.format;
	}

	VkImageLayout TextureResource::GetLayout() const
	{
		return layout;
	}

	bool TextureResource::IsInvalid() const
	{
		return !isValid;
	}

	bool TextureResource::IsDepthTexture() const
	{
		switch (baseImageInfo.format)
		{
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
		case VK_FORMAT_D16_UNORM_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT:
		case VK_FORMAT_D16_UNORM:
		{
			return true;
		}
		}

		return false;
	}

	bool TextureResource::HasStencilComponent() const
	{
		switch (baseImageInfo.format)
		{
		case VK_FORMAT_D16_UNORM_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
		{
			return true;
		}
		}
		
		return false;
	}

	uint32_t TextureResource::GetWidth() const
	{
		return baseImageInfo.width;
	}

	uint32_t TextureResource::GetHeight() const
	{
		return baseImageInfo.height;
	}

	uint32_t TextureResource::GetAllocatedMipLevels() const
	{
		return baseImageInfo.mipLevels;
	}

	uint32_t TextureResource::GetGeneratedMipLevels() const
	{
		return generatedMips;
	}

	uint32_t TextureResource::CalculateMipLevelsFromSize() const
	{
		return CalculateMipLevelsFromSize(baseImageInfo.width, baseImageInfo.height);
	}

	ImageViewScope TextureResource::GetViewScope() const
	{
		return imageViewInfo.viewScope;
	}

	void TextureResource::CreateBaseImage(const BaseImageCreateInfo* _baseImageInfo)
	{
		CreateBaseImage_Helper(_baseImageInfo);
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

		BaseImageCreateInfo _baseImageInfo = *createInfo;
		_baseImageInfo.width = _width;
		_baseImageInfo.height = _height;
		CreateBaseImage_Helper(&_baseImageInfo);

		VkDeviceSize imageSize = _width * _height * bytesPerPixel;
		CopyFromData(data, imageSize);

		// Now that we've copied over the data to the texture image, we don't need the original pixels array anymore
		stbi_image_free(data);

		TransitionLayout_Immediate(layout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void TextureResource::GenerateMipmaps_Helper(VkCommandBuffer cmdBuffer, uint32_t mipCount)
	{
		if (IsInvalid())
		{
			LogError("Attempting to generate mipmaps but base image has not yet been created!");
			return;
		}

		// Not enough allocated mip levels
		if (mipCount > baseImageInfo.mipLevels)
		{
			return;
		}

		// We've already generated all the mips, no point in doing it again
		if (generatedMips >= mipCount)
		{
			return;
		}

		VkPhysicalDevice physicalDevice = GetPhysicalDevice();

		// Check if the texture format we want to use supports linear blitting
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, baseImageInfo.format, &formatProperties);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		{
			TNG_ASSERT_MSG(false, "Texture image does not support linear blitting!");
		}

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = baseImage;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = generatedMips;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = baseImageInfo.arrayLayers;

		uint32_t mipWidth = baseImageInfo.width >> (generatedMips - 1);
		uint32_t mipHeight = baseImageInfo.height >> (generatedMips - 1);

		for (uint32_t i = generatedMips; i < mipCount; i++)
		{
			// Transition image from transfer dst optimal to transfer src optimal, since we're reading from this image
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(cmdBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { static_cast<int32_t>(mipWidth), static_cast<int32_t>(mipHeight), 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = baseImageInfo.arrayLayers;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { static_cast<int32_t>(mipWidth) > 1 ? static_cast<int32_t>(mipWidth >> 1) : 1, static_cast<int32_t>(mipHeight) > 1 ? static_cast<int32_t>(mipHeight >> 1) : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = baseImageInfo.arrayLayers;

			vkCmdBlitImage(cmdBuffer,
				baseImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				baseImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			// Transition image from src transfer optimal to shader read only
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(cmdBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1u) mipWidth >>= 1;
			if (mipHeight > 1u) mipHeight >>= 1;
		}

		// Transfer the last mip level to shader read-only because this wasn't handled by the loop above
		// (since we didn't blit from the last image)
		barrier.subresourceRange.baseMipLevel = mipCount - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmdBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		// Success!
		layout = barrier.newLayout;
		generatedMips = mipCount;
	}

	void TextureResource::CreateImageViews(const ImageViewCreateInfo* _viewInfo)
	{
		if (IsInvalid())
		{
			LogError("Attempting to create image view(s), but base image has not yet been created!");
			return;
		}

		if (imageViews.size() > 0)
		{
			// This is just a warning because RecreateImageViews will actually clean up the existing image views.
			// When this function is called there usually aren't any image views to begin with.
			LogWarning("Attempting to create image view(s), but image views have already been created!");
		}

		RecreateImageViews(_viewInfo);

	}

	void TextureResource::CreateSampler(const SamplerCreateInfo* _samplerInfo)
	{
		if (_samplerInfo->enableAnisotropicFiltering && _samplerInfo->maxAnisotropy == 1.0)
		{
			LogWarning("Anisotropy is enabled for texture resource, but it's max level is set to 1.0. This effectively disables anisotropy. Consider disabling anisotropic filtering or increase max anisotropy!");
		}

		VkDevice logicalDevice = GetLogicalDevice();

		VkSamplerCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.magFilter = _samplerInfo->magnificationFilter;
		createInfo.minFilter = _samplerInfo->minificationFilter;
		createInfo.addressModeU = _samplerInfo->addressModeUVW;
		createInfo.addressModeV = _samplerInfo->addressModeUVW;
		createInfo.addressModeW = _samplerInfo->addressModeUVW;
		createInfo.anisotropyEnable = _samplerInfo->enableAnisotropicFiltering;
		createInfo.maxAnisotropy = _samplerInfo->maxAnisotropy;
		createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		createInfo.unnormalizedCoordinates = VK_FALSE;
		createInfo.compareEnable = VK_FALSE;
		createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		createInfo.mipmapMode = _samplerInfo->mipmapMode;
		createInfo.mipLodBias = 0.0f;
		createInfo.minLod = 0.0f;
		createInfo.maxLod = static_cast<float>(baseImageInfo.mipLevels);

		if (vkCreateSampler(logicalDevice, &createInfo, nullptr, &sampler) != VK_SUCCESS)
		{
			LogError(false, "Failed to create texture sampler!");
			return;
		}

		samplerInfo = *_samplerInfo;
	}

	void TextureResource::CreateBaseImage_Helper(const BaseImageCreateInfo* _baseImageInfo)
	{
		VkDevice logicalDevice = GetLogicalDevice();

		// Re-calculate mip count, if necessary. Vulkan disallows 0 mip levels
		uint32_t mipsToUse = 1;
		if (_baseImageInfo->mipLevels == 0)
		{
			mipsToUse = CalculateMipLevelsFromSize(_baseImageInfo->width, _baseImageInfo->height);
			LogWarning("Texture resource specified an invalid 0 mip levels for a %ux%u, using %u mip levels instead", _baseImageInfo->width, _baseImageInfo->height, mipsToUse);
		}
		else
		{
			mipsToUse = _baseImageInfo->mipLevels;
		}

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = _baseImageInfo->width;
		imageInfo.extent.height = _baseImageInfo->height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipsToUse;
		imageInfo.arrayLayers = _baseImageInfo->arrayLayers;
		imageInfo.format = _baseImageInfo->format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = _baseImageInfo->usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = _baseImageInfo->samples;
		imageInfo.flags = _baseImageInfo->flags;

		// Create the image
		if (vkCreateImage(logicalDevice, &imageInfo, nullptr, &baseImage) != VK_SUCCESS)
		{
			LogError("Failed to create image!");
			return;
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(logicalDevice, baseImage, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		{
			LogError("Failed to allocate image memory!");
			return; // No need to clean-up image since we couldn't allocate the memory anyway
		}

		vkBindImageMemory(logicalDevice, baseImage, imageMemory, 0);

		// Cache some of the image data
		bytesPerPixel = GetBytesPerPixelFromFormat(_baseImageInfo->format);
		baseImageInfo = *_baseImageInfo;
		baseImageInfo.mipLevels = mipsToUse;
		isValid = true;
		generatedMips = 1;

		// Calculate mip levels
		if (baseImageInfo.generateMipMaps && mipsToUse > 1)
		{
			// Source layout should always be UNDEFINED here
			TransitionLayout_Immediate(layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			GenerateMipmaps_Immediate(_baseImageInfo->mipLevels);
		}
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

		// NOTE - This function will only ever create a single image view for the entire image. This is equivalent
		//        to calling Create() and passing in viewInfo->viewScope as ImageViewScope::ENTIRE_IMAGE
		imageViews.resize(1);
		VkImageView& imageView = imageViews.at(0);
		if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &imageView) != VK_SUCCESS)
		{
			LogError(false, "Failed to create texture image view!");
		}
	}

	void TextureResource::CopyFromData(void* data, VkDeviceSize bytes)
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
		VkDeviceSize imageSize = baseImageInfo.width * baseImageInfo.height * bytesPerPixel;
		if (bytes > imageSize)
		{
			LogWarning("Attempting to copy %u bytes into texture image, when the image size is only %u. Only %u bytes will be copied.", bytes, imageSize, imageSize);
		}
		
		VkDeviceSize actualSize = std::min(bytes, imageSize);

		StagingBuffer stagingBuffer;
		stagingBuffer.Create(actualSize);
		stagingBuffer.CopyIntoBuffer(data, actualSize);

		VkImageLayout oldLayout = layout;

		TransitionLayout_Immediate(oldLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		CopyFromBuffer(stagingBuffer.GetBuffer(), 0);

		if (oldLayout != VK_IMAGE_LAYOUT_UNDEFINED)
		{
			TransitionLayout_Immediate(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, oldLayout);
		}

		stagingBuffer.Destroy();
	}

	void TextureResource::CopyFromTexture(CommandBuffer* cmdBuffer, TextureResource* sourceTexture, uint32_t baseMip, uint32_t mipCount)
	{
		// Either source or destination (this) textures are invalid
		if (sourceTexture == nullptr || baseImage == VK_NULL_HANDLE)
		{
			return;
		}

		// No work to be done
		if (mipCount == 0 || sourceTexture == this)
		{
			return;
		}

		if (sourceTexture->baseImageInfo.arrayLayers != baseImageInfo.arrayLayers)
		{
			LogWarning("Failed to copy from texture. Mismatched array layers: source (%u) vs. destination (%u)", sourceTexture->baseImageInfo.arrayLayers, baseImageInfo.arrayLayers);
			return;
		}

		if ((baseMip + mipCount) > baseImageInfo.mipLevels || (baseMip + mipCount) > sourceTexture->baseImageInfo.mipLevels)
		{
			LogWarning("Failed to copy from texture. Specified mips (%u base + %u count) are higher than the number of allocated mips in the source and/or destination texture: source (%u) vs. destination (%u)!",
				baseMip,
				mipCount,
				sourceTexture->baseImageInfo.mipLevels,
				baseImageInfo.mipLevels
			);
		}

		if (mipCount > sourceTexture->baseImageInfo.mipLevels || mipCount > baseImageInfo.mipLevels)
		{
			LogWarning("Failed to copy from texture. Requested more mips than are allocated in source and/or destination textures: source (%u) vs. destination (%u).",
				sourceTexture->baseImageInfo.mipLevels, baseImageInfo.mipLevels);
			return;
		}

		VkImageLayout oldDestinationLayout = layout;
		VkImageLayout oldSourceLayout = sourceTexture->layout;

		// Transition the source image to TRANSFER_SRC_OPTIMAL and the destination to TRANSFER_DST_OPTIMAL
		TransitionLayout_Internal(cmdBuffer->GetBuffer(), sourceTexture, sourceTexture->layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		TransitionLayout_Internal(cmdBuffer->GetBuffer(), this, layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		uint32_t mipWidth = baseImageInfo.width >> baseMip;
		uint32_t mipHeight = baseImageInfo.height >> baseMip;

		VkImageCopy copyRegion{};
		copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.srcSubresource.baseArrayLayer = 0;
		copyRegion.srcSubresource.layerCount = sourceTexture->baseImageInfo.arrayLayers;
		copyRegion.srcOffset = { 0, 0, 0 };
		copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.dstSubresource.baseArrayLayer = 0;
		copyRegion.dstSubresource.layerCount = baseImageInfo.arrayLayers;
		copyRegion.dstOffset = { 0, 0, 0 };

		std::vector<VkImageCopy> regions;
		regions.reserve(mipCount);

		// Generate all the regions we're going to copy over
		for (uint32_t i = 0; i < mipCount; i++)
		{
			copyRegion.srcSubresource.mipLevel = baseMip + i;
			copyRegion.dstSubresource.mipLevel = baseMip + i;
			copyRegion.extent = { mipWidth, mipHeight, 1u };

			regions.push_back(copyRegion);

			if (mipWidth > 1) mipWidth >>= 1;
			if (mipHeight > 1) mipHeight >>= 1;
		}

		vkCmdCopyImage(cmdBuffer->GetBuffer(),
			sourceTexture->baseImage, sourceTexture->layout,
			baseImage, layout,
			static_cast<uint32_t>(regions.size()), regions.data());

		// Transfer the textures to their old layouts, unless their old layout was UNDEFINED
		if (oldSourceLayout != VK_IMAGE_LAYOUT_UNDEFINED)		TransitionLayout_Internal(cmdBuffer->GetBuffer(), sourceTexture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, oldSourceLayout);
		if (oldDestinationLayout != VK_IMAGE_LAYOUT_UNDEFINED)	TransitionLayout_Internal(cmdBuffer->GetBuffer(), this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, oldDestinationLayout);
	
		// We didn't technically "generate" them, but for all practical purposes this is 
		// the amount of valid mips we have
		// TODO - Track which mips were generated now that we can copy from specific mip levels
		generatedMips = mipCount;
	}


	void TextureResource::RecreateImageViews(const ImageViewCreateInfo* _viewInfo)
	{
		DestroyImageViews();

		VkDevice logicalDevice = GetLogicalDevice();

		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = baseImage;
		createInfo.viewType = _viewInfo->viewType;
		createInfo.format = baseImageInfo.format;
		createInfo.subresourceRange.aspectMask = _viewInfo->aspect;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = baseImageInfo.mipLevels;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		// Consider view type
		if (_viewInfo->viewType == VK_IMAGE_VIEW_TYPE_CUBE)
		{
			createInfo.subresourceRange.layerCount = 6;
		}

		// Consider view scope
		switch (_viewInfo->viewScope)
		{
		case ImageViewScope::ENTIRE_IMAGE:
		{
			imageViews.resize(1);
			VkImageView& imageView = imageViews.at(0);
			if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &imageView) != VK_SUCCESS)
			{
				LogError("Failed to create texture image view!");
			}

			break;
		}
		case ImageViewScope::PER_MIP_LEVEL:
		{
			createInfo.subresourceRange.levelCount = 1;

			imageViews.resize(baseImageInfo.mipLevels);
			for (uint32_t i = 0; i < baseImageInfo.mipLevels; i++)
			{
				// We set the base mip level to the current mip level in the iteration and levelCount will remain as 1
				createInfo.subresourceRange.baseMipLevel = i;

				VkImageView& imageView = imageViews.at(i);
				if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &imageView) != VK_SUCCESS)
				{
					LogError("Failed to create texture image view!");
				}
			}

			break;
		}
		}

		imageViewInfo = *_viewInfo;
	}

	void TextureResource::CopyFromBuffer(VkBuffer buffer, uint32_t destinationMipLevel)
	{
		if (IsInvalid())
		{
			LogError("Attempting to copy from buffer, but base image has not yet been created!");
			return;
		}

		{
			DisposableCommand command(QueueType::TRANSFER, true);

			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;

			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = destinationMipLevel;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = baseImageInfo.arrayLayers;

			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = { baseImageInfo.width, baseImageInfo.height, 1 };

			vkCmdCopyBufferToImage(command.GetBuffer(), buffer, baseImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		}
	}

	void TextureResource::TransitionLayout_Internal(VkCommandBuffer commandBuffer, TextureResource* baseTexture, VkImageLayout sourceLayout, VkImageLayout destinationLayout)
	{
		VkPipelineStageFlags sourceStage = 0;
		VkPipelineStageFlags destinationStage = 0;
		QueueType queueType = QueueType::GRAPHICS;

		auto barrier = TransitionLayout_Helper(baseTexture, sourceLayout, destinationLayout, sourceStage, destinationStage, queueType);
		if (!barrier.has_value())
		{
			return;
		}

		InsertPipelineBarrier_Internal(commandBuffer, sourceStage, destinationStage, barrier.value());

		// Success!
		baseTexture->layout = destinationLayout;
	}

	std::optional<VkImageMemoryBarrier> TextureResource::TransitionLayout_Helper(const TextureResource* baseTexture, 
		VkImageLayout sourceLayout,
		VkImageLayout destinationLayout,
		VkPipelineStageFlags& out_sourceStage,
		VkPipelineStageFlags& out_destinationStage,
		QueueType& out_queueType)
	{
		if (baseTexture && baseTexture->IsInvalid())
		{
			LogError("Attempting to transition layout of invalid texture!");
			return {};
		}

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = sourceLayout;
		barrier.newLayout = destinationLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = baseTexture->baseImage;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = baseTexture->baseImageInfo.mipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = baseTexture->baseImageInfo.arrayLayers;
		barrier.srcAccessMask = 0; // These are set below
		barrier.dstAccessMask = 0; // These are set below

		VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_NONE;
		VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_NONE;
		QueueType commandQueueType = QueueType::GRAPHICS;

		if (destinationLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (HasStencilComponent())
			{
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		if (sourceLayout == VK_IMAGE_LAYOUT_UNDEFINED && destinationLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			commandQueueType = QueueType::TRANSFER;
		}
		else if (sourceLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && destinationLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			commandQueueType = QueueType::TRANSFER;
		}
		else if (sourceLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && destinationLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			commandQueueType = QueueType::TRANSFER;
		}
		else if (sourceLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && destinationLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			commandQueueType = QueueType::GRAPHICS;
		}
		else if (sourceLayout == VK_IMAGE_LAYOUT_UNDEFINED && destinationLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT; // Let's block on the vertex shader for now...

			commandQueueType = QueueType::GRAPHICS;
		}
		else if (sourceLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && destinationLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // I guess this depends on whether we're using the texture in the vertex or pixel shader?

			commandQueueType = QueueType::GRAPHICS;
		}
		else if (sourceLayout == VK_IMAGE_LAYOUT_UNDEFINED && destinationLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

			commandQueueType = QueueType::GRAPHICS;
		}
		else if (sourceLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && destinationLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			// We're probably converting the color attachment after doing the LDR conversion

			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			commandQueueType = QueueType::GRAPHICS;
		}
		else if (sourceLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && destinationLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			commandQueueType = QueueType::GRAPHICS;
		}
		else if (sourceLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && destinationLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			commandQueueType = QueueType::GRAPHICS;
		}
		else if (sourceLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && destinationLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			// We're probably converting the color attachment before doing the LDR conversion

			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			commandQueueType = QueueType::GRAPHICS;
		}
		else if (sourceLayout == VK_IMAGE_LAYOUT_UNDEFINED && destinationLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			commandQueueType = QueueType::GRAPHICS;
		}
		else if (sourceLayout == VK_IMAGE_LAYOUT_UNDEFINED && destinationLayout == VK_IMAGE_LAYOUT_GENERAL)
		{
			// Textures transitioning from these layouts usually transitioned immediately using a disposable command
			// Do access masks and stages really matter that much in this case??
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = 0;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

			commandQueueType = QueueType::GRAPHICS;
		}
		else if (sourceLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && destinationLayout == VK_IMAGE_LAYOUT_GENERAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = 0;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

			commandQueueType = QueueType::GRAPHICS;
		}
		else if (sourceLayout == VK_IMAGE_LAYOUT_GENERAL && destinationLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			commandQueueType = QueueType::GRAPHICS;
		}
		else if (sourceLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && destinationLayout == VK_IMAGE_LAYOUT_GENERAL)
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

			commandQueueType = QueueType::GRAPHICS;
		}
		else if (sourceLayout == VK_IMAGE_LAYOUT_GENERAL && destinationLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			commandQueueType = QueueType::GRAPHICS;
		}
		else if (sourceLayout == VK_IMAGE_LAYOUT_GENERAL && destinationLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			commandQueueType = QueueType::GRAPHICS;
		}
		else if (sourceLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && destinationLayout == VK_IMAGE_LAYOUT_GENERAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

			commandQueueType = QueueType::GRAPHICS;
		}
		else
		{
			TNG_ASSERT_MSG(false, "Unsupported layout transition!");
		}

		out_sourceStage = sourceStage;
		out_destinationStage = destinationStage;
		out_queueType = commandQueueType;

		return barrier;
	}

	void TextureResource::InsertPipelineBarrier_Helper(VkCommandBuffer cmdBuffer,
		VkAccessFlags srcAccessFlags,
		VkAccessFlags dstAccessFlags,
		VkPipelineStageFlags srcStage,
		VkPipelineStageFlags dstStage,
		uint32_t baseMip,
		uint32_t mipCount)
	{
		// This barrier preserves the current image layout
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = layout;
		barrier.newLayout = layout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = baseImage;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = baseMip;
		barrier.subresourceRange.levelCount = mipCount;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = baseImageInfo.arrayLayers;
		barrier.srcAccessMask = srcAccessFlags;
		barrier.dstAccessMask = dstAccessFlags;

		InsertPipelineBarrier_Internal(cmdBuffer, srcStage, dstStage, barrier);
	}

	void TextureResource::InsertPipelineBarrier_Internal(VkCommandBuffer cmdBuffer, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkImageMemoryBarrier barrier)
	{
		vkCmdPipelineBarrier(
			cmdBuffer,
			srcStage, dstStage,
			0,
			0, nullptr, // No memory barriers
			0, nullptr, // No buffer barriers
			1, &barrier
		);
	}

	void TextureResource::ResetMembers()
	{
		name = "";
		isValid = false;
		bytesPerPixel = 0;
		layout = VK_IMAGE_LAYOUT_UNDEFINED;
		generatedMips = 0;

		baseImageInfo = BaseImageCreateInfo();
		imageViewInfo = ImageViewCreateInfo();
		samplerInfo = SamplerCreateInfo();

		baseImage = VK_NULL_HANDLE;
		imageMemory = VK_NULL_HANDLE;
		imageViews.clear();
		sampler = VK_NULL_HANDLE;
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

		LogError("Failed to find suitable memory type!");
		return std::numeric_limits<uint32_t>::max();
	}

	uint32_t TextureResource::GetBytesPerPixelFromFormat(VkFormat texFormat)
	{
		switch (texFormat)
		{
		case VK_FORMAT_R16G16_SFLOAT:
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

	uint32_t TextureResource::CalculateMipLevelsFromSize(uint32_t _width, uint32_t _height) const
	{
		double dWidth = static_cast<double>(_width);
		double dHeight = static_cast<double>(_height);
		double exactMips = log2(std::min(dWidth, dHeight));
		return static_cast<uint32_t>(exactMips);
	}
}