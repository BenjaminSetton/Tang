
#include "descriptor_pool.h"

#include "../device_cache.h"
#include "../utils/sanity_check.h"
#include "../utils/logger.h"

namespace TANG
{
	// Assert that the DescriptorPool class remains the same size as the underlying VkDescriptorPool
	TNG_ASSERT_SAME_SIZE(sizeof(DescriptorPool), sizeof(VkDescriptorPool));

	DescriptorPool::DescriptorPool() : pool(VK_NULL_HANDLE)
	{
	}

	DescriptorPool::~DescriptorPool()
	{
		// Nothing to do here
	}

	DescriptorPool::DescriptorPool(const DescriptorPool& other)
	{
		LogInfo("Copy constructor for descriptor pool invoked!");
		pool = other.pool;
	}

	DescriptorPool::DescriptorPool(DescriptorPool&& other)
	{
		LogInfo("Move constructor for descriptor pool invoked!");
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

		LogInfo("Assignment operator for descriptor pool invoked!");
		pool = other.pool;
		return *this;
	}

	void DescriptorPool::Create(VkDescriptorPoolSize* poolSizes, uint32_t poolSizeCounts, uint32_t maxSets, VkDescriptorPoolCreateFlags flags)
	{
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = poolSizeCounts;
		poolInfo.pPoolSizes = poolSizes;
		poolInfo.maxSets = maxSets;
		poolInfo.flags = flags;

		if (vkCreateDescriptorPool(GetLogicalDevice(), &poolInfo, nullptr, &pool) != VK_SUCCESS) {
			TNG_ASSERT_MSG(false, "Failed to create descriptor pool!");
		}
	}

	void DescriptorPool::Reset()
	{
		vkResetDescriptorPool(GetLogicalDevice(), pool, 0);
	}

	void DescriptorPool::Destroy()
	{
		vkDestroyDescriptorPool(GetLogicalDevice(), pool, nullptr);
	}

	VkDescriptorPool DescriptorPool::GetPool() const
	{
		return pool;
	}

	bool DescriptorPool::IsValid()
	{
		return pool != VK_NULL_HANDLE;
	}

	void DescriptorPool::ClearHandle()
	{
		if (pool == VK_NULL_HANDLE)
		{
			LogError("Attempting to clear handle of a descriptor pool when the handle doesn't exist!");
		}

		pool = VK_NULL_HANDLE;
	}

}