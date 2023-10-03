#ifndef WRITE_DESCRIPTOR_SET_H
#define WRITE_DESCRIPTOR_SET_H

#include <vector>

#include "vulkan/vulkan.h"

namespace TANG
{
	// Encapsulates the data and functionality for creating a WriteDescriptorSet
	// Note that it's not allowed to copy an object of this class, it should only be moved
	class WriteDescriptorSets
	{
	public:

		WriteDescriptorSets(uint32_t bufferCount, uint32_t imageCount);
		~WriteDescriptorSets();
		WriteDescriptorSets(const WriteDescriptorSets& other) = delete;
		WriteDescriptorSets(WriteDescriptorSets&& other);
		WriteDescriptorSets& operator=(const WriteDescriptorSets& other) = delete;

		void AddUniformBuffer(VkDescriptorSet descriptorSet, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize, VkDeviceSize offset);
		void AddImageSampler(VkDescriptorSet descriptorSet, uint32_t binding, VkImageView imageView, VkSampler sampler);

		uint32_t GetWriteDescriptorSetCount() const;
		const VkWriteDescriptorSet* GetWriteDescriptorSets() const;

	private:

		std::vector<VkWriteDescriptorSet> writeDescriptorSets;

		uint32_t numBuffers = 0;
		uint32_t numImages = 0;

		// Temporary memory, since a VkWriteDescriptorSet object requires a pointer to these structs
		std::vector<VkDescriptorImageInfo> descriptorImageInfo;
		std::vector<VkDescriptorBufferInfo> descriptorBufferInfo;

	};
}

#endif