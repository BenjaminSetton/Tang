
#include "write_descriptor_set.h"

#include "../utils/logger.h"

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

	void WriteDescriptorSets::AddUniformBuffer(VkDescriptorSet descriptorSet, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize, VkDeviceSize offset)
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
		bufferInfo.buffer = buffer;
		bufferInfo.offset = offset;
		bufferInfo.range = bufferSize;

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

	void WriteDescriptorSets::AddImageSampler(VkDescriptorSet descriptorSet, uint32_t binding, VkImageView imageView, VkSampler sampler)
	{
		if (numImages == 0)
		{
			// We'll return in this case because the internal temporary vectors that hold the buffers and images will be forced to
			// resize if we add this next image sampler. This will cause all the pointers to become invalid and we'll eventually
			// crash
			LogError("Failed to add image sampler to WriteDescriptorSet. Exceeded the number of promised image samplers!");
			return;
		}

		descriptorImageInfo.emplace_back(std::move(VkDescriptorImageInfo()));
		VkDescriptorImageInfo& imageInfo = descriptorImageInfo.back();
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imageInfo.imageView = imageView;
		imageInfo.sampler = sampler;

		VkWriteDescriptorSet writeDescSet{};
		writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescSet.dstSet = descriptorSet;
		writeDescSet.dstBinding = binding;
		writeDescSet.dstArrayElement = 0;
		writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
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
}