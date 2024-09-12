#ifndef DESCRIPTOR_POOL_H
#define DESCRIPTOR_POOL_H

#include "../utils/sanity_check.h"
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

		void Create(VkDescriptorPoolSize* poolSizes, uint32_t poolSizeCounts, uint32_t maxSets, VkDescriptorPoolCreateFlags flags);
		void Reset();
		void Destroy();

		VkDescriptorPool GetPool() const;
		bool IsValid();
		void ClearHandle();

	private:

		VkDescriptorPool pool;

	};

	// A thin wrapper around a VkDescriptorPool. Contains no object other than the vulkan pool object itself, meaning that an array of
	// VkDescriptorPool objects is equivalent to an array of DescriptorPool objects
	TNG_ASSERT_SAME_SIZE(DescriptorPool, VkDescriptorPool);
}

#endif