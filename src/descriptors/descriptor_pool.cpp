
#include "descriptor_pool.h"

#include "../utils/sanity_check.h"
#include "../utils/logger.h"

namespace TANG
{

	DescriptorPool::DescriptorPool() : pool(VK_NULL_HANDLE)
	{

	}

	DescriptorPool::~DescriptorPool()
	{
		// Nothing to do here
	}

	DescriptorPool::DescriptorPool(const DescriptorPool& other)
	{
		// Is this necessary??
		LogWarning("Copy constructor for descriptor pool invoked!");
		pool = other.pool;
	}

	DescriptorPool::DescriptorPool(DescriptorPool&& other)
	{
		// Is this necessary??
		LogWarning("Move constructor for descriptor pool invoked!");
		pool = other.pool;

		other.pool = VK_NULL_HANDLE;
	}

	DescriptorPool& DescriptorPool::operator=(const DescriptorPool& other)
	{
		// Protect against self-assignment
		if (this == &other)
		{
			return *this;
		}

		// Is this necessary??
		LogWarning("Assignment operator for descriptor pool invoked!");
		pool = other.pool;
		return *this;
	}

	void DescriptorPool::Create(VkDevice logicalDevice, VkDescriptorPoolSize* poolSizes, uint32_t poolSizeCounts, uint32_t maxSets)
	{
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = poolSizeCounts;
		poolInfo.pPoolSizes = poolSizes;
		poolInfo.maxSets = maxSets;

		if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
			TNG_ASSERT_MSG(false, "Failed to create descriptor pool!");
		}
	}

	void DescriptorPool::Destroy(VkDevice logicalDevice)
	{
		vkDestroyDescriptorPool(logicalDevice, pool, nullptr);
	}

	VkDescriptorPool DescriptorPool::GetPool()
	{
		return pool;
	}

}