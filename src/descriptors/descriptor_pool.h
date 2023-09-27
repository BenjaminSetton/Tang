#ifndef DESCRIPTOR_POOL_H
#define DESCRIPTOR_POOL_H

#include "vulkan/vulkan.h"

namespace TANG
{
	class DescriptorPool
	{
	public:

		DescriptorPool();
		~DescriptorPool();
		DescriptorPool(const DescriptorPool& other);
		DescriptorPool(DescriptorPool&& other);
		DescriptorPool& operator=(const DescriptorPool& other);

		void Create(VkDevice logicalDevice, VkDescriptorPoolSize* poolSizes, uint32_t poolSizeCounts, uint32_t maxSets);
		void Destroy(VkDevice logicalDevice);

		VkDescriptorPool GetPool();

	private:

		VkDescriptorPool pool;

	};
}

#endif