
#include "../utils/logger.h"
#include "../data_buffer/uniform_buffer.h"
#include "write_descriptor_set.h"

namespace TANG
{
	WriteDescriptorSets::WriteDescriptorSets(uint32_t bufferCount, uint32_t imageCount)
	{
		numBuffers = bufferCount;
		numImages = imageCount;

		descriptorBufferInfo.reserve(numBuffers);
		descriptorImageInfo.reserve(numImages);
	}

	WriteDescriptorSets::~WriteDescriptorSets()
	{
		descriptorImageInfo.clear();
		descriptorBufferInfo.clear();
		writeDescriptorSets.clear();

		numBuffers = 0;
		numImages = 0;
	}

	WriteDescriptorSets::WriteDescriptorSets(WriteDescriptorSets&& other) :
		writeDescriptorSets(std::move(other.writeDescriptorSets)), descriptorBufferInfo(std::move(descriptorBufferInfo)), descriptorImageInfo(std::move(descriptorImageInfo)),
		numBuffers(other.numBuffers), numImages(other.numImages)
	{
		other.numBuffers = 0;
		other.numImages = 0;
	}

	void WriteDescriptorSets::AddUniformBuffer(VkDescriptorSet descriptorSet, uint32_t binding, const UniformBuffer* uniformBuffer, VkDeviceSize offset)
	{
		if (numBuffers == 0)
		{
			// We'll return in this case because the internal temporary vectors that hold the buffers and images will be forced to
			// resize if we add this next uniform buffer. This will cause all the pointers to become invalid and we'll eventually
			// crash
			LogError("Failed to add uniform buffer to WriteDescriptorSet. Exceeded the number of promised uniform buffers!");
			return;
		}

		descriptorBufferInfo.push_back(VkDescriptorBufferInfo());
		VkDescriptorBufferInfo& bufferInfo = descriptorBufferInfo.back();
		bufferInfo.buffer = uniformBuffer->GetBuffer();
		bufferInfo.offset = offset;
		bufferInfo.range = uniformBuffer->GetBufferSize();

		VkWriteDescriptorSet writeDescSet{};
		writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescSet.dstSet = descriptorSet;
		writeDescSet.dstBinding = binding;
		writeDescSet.dstArrayElement = 0;
		writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescSet.descriptorCount = 1;
		writeDescSet.pBufferInfo = &bufferInfo;

		writeDescriptorSets.push_back(writeDescSet);

		// Decrement the number of buffers the caller promised to write to
		numBuffers--;
	}

	void WriteDescriptorSets::AddImage(VkDescriptorSet descriptorSet, uint32_t binding, const TextureResource* texResource, VkDescriptorType type, uint32_t imageViewIndex)
	{
		// We'll return in this case because the internal temporary vectors that hold the buffers and images will be forced to
		// resize if we add this next image sampler. This will cause all the pointers to become invalid and we'll eventually
		// crash
		if (numImages == 0)
		{
			LogError("Failed to update image descriptor. Exceeded the number of promised image samplers!");
			return;
		}

		// Log an error message if we already have a write for the specified binding. At the moment we don't
		// bundle up descriptor set writes, so looping over the vector should not be a problem...for now
		for (const auto& writeDescSet : writeDescriptorSets)
		{
			if (writeDescSet.dstBinding == binding)
			{
				LogError("A descriptor set write was already specified for binding %u! This write will be ignored", binding);
				return;
			}
		}

		VkImageLayout layout = texResource->GetLayout();
		if (layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && layout != VK_IMAGE_LAYOUT_GENERAL)
		{
			LogWarning("Attempting to update image descriptor on binding %u with texture resource that does not have a layout appropriate for shader read/write operations!", binding);
		}

		if (texResource->GetGeneratedMipLevels() > 1 && texResource->GetViewScope() == ImageViewScope::PER_MIP_LEVEL)
		{
			LogWarning("Attempting to update image descriptor on binding %u which has more than 1 generated mip level, but it's view scope is declared per mip level! Image sampling will exclusively read from mip level 0", binding);
		}

		descriptorImageInfo.emplace_back(std::move(VkDescriptorImageInfo()));
		VkDescriptorImageInfo& imageInfo = descriptorImageInfo.back();
		imageInfo.imageLayout = layout;
		imageInfo.imageView = texResource->GetImageView(imageViewIndex); // Index 0 represent either the entire image (ImageViewScope::ENTIRE_IMAGE) or the first mip level (highest quality/resolution - ImageViewScope::PER_MIP_LEVEL)
		imageInfo.sampler = texResource->GetSampler();

		VkWriteDescriptorSet writeDescSet{};
		writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescSet.dstSet = descriptorSet;
		writeDescSet.dstBinding = binding;
		writeDescSet.dstArrayElement = 0;
		writeDescSet.descriptorType = type;
		writeDescSet.descriptorCount = 1;
		writeDescSet.pImageInfo = &imageInfo;

		writeDescriptorSets.push_back(writeDescSet);

		// Decrement the number of samplers the caller promised to write to
		numImages--;
	}

	uint32_t WriteDescriptorSets::GetWriteDescriptorSetCount() const
	{
		return static_cast<uint32_t>(writeDescriptorSets.size());
	}

	const VkWriteDescriptorSet* WriteDescriptorSets::GetWriteDescriptorSets() const
	{
		return writeDescriptorSets.data();
	}

	uint32_t WriteDescriptorSets::GetRemainingUniformBufferSlots() const
	{
		return numBuffers;
	}

	uint32_t WriteDescriptorSets::GetRemainingImageSamplerSlots() const
	{
		return numImages;
	}
}