#ifndef DESCRIPTOR_POOL_H
#define DESCRIPTOR_POOL_H

#include "vulkan/vulkan.h"

namespace TANG
{
	// A thin wrapper around a VkDescriptorPool. Contains no object other than the vulkan pool object itself, meaning that an array of
	// VkDescriptorPool objects is equivalent to an array of DescriptorPool objects
	class DescriptorPool
	{
	public:

		DescriptorPool();
		~DescriptorPool();
		DescriptorPool(const DescriptorPool& other);
		DescriptorPool(DescriptorPool&& other);
		DescriptorPool& operator=(const DescriptorPool& other);

		void Create(VkDevice logicalDevice, VkDescriptorPoolSize* poolSizes, uint32_t poolSizeCounts, uint32_t maxSets, VkDescriptorPoolCreateFlags flags);
		void Reset(VkDevice logicalDevice);
		void Destroy(VkDevice logicalDevice);

		VkDescriptorPool GetPool();
		bool IsValid();
		void ClearHandle();

	private:

		VkDescriptorPool pool;

	};
}

#endif