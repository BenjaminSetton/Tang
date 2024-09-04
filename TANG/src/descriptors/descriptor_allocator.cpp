
#include "../device_cache.h"
#include "../utils/logger.h"
#include "../utils/sanity_check.h"
#include "descriptor_allocator.h"
#include "descriptor_pool.h"
#include "descriptor_set.h"
#include "set_layout/set_layout.h"

namespace TANG
{

	DescriptorAllocator::DescriptorAllocator() : currentPool(DescriptorPool())
	{
	}


	DescriptorAllocator::~DescriptorAllocator()
	{
		if (usedPools.size() != 0 || freePools.size() != 0)
		{
			LogWarning("Descriptor allocator pools were not properly freed!");
		}

		// Nothing for the destructor to do. Memory should be freed properly in DestroyPools()
	}

	DescriptorAllocator::DescriptorAllocator(const DescriptorAllocator& other) :
		currentPool(other.currentPool), descriptorSizes(other.descriptorSizes),
		usedPools(other.usedPools), freePools(other.freePools)
	{
		// Nothing to do here
	}

	DescriptorAllocator::DescriptorAllocator(DescriptorAllocator&& other) noexcept :
		currentPool(std::move(other.currentPool)), descriptorSizes(std::move(other.descriptorSizes)),
		usedPools(std::move(other.usedPools)), freePools(std::move(other.freePools))
	{
		other.currentPool.ClearHandle();
		other.descriptorSizes.sizes.clear();
		other.usedPools.clear();
		other.freePools.clear();
	}

	DescriptorAllocator& DescriptorAllocator::operator=(const DescriptorAllocator& other)
	{
		// Protect against self-assignment
		if (this == &other)
		{
			return *this;
		}

		currentPool = other.currentPool;
		descriptorSizes = other.descriptorSizes;
		usedPools = other.usedPools;
		freePools = other.freePools;

		return *this;
	}

	bool DescriptorAllocator::CreateSet(DescriptorSetLayout setLayout, DescriptorSet* out_set)
	{
		TNG_ASSERT_MSG(out_set != nullptr, "Cannot create a descriptor set when the handle is null!");

		// Grab a pool, if we don't have one already
		if (!currentPool.IsValid())
		{
			currentPool = PickPool();
			usedPools.push_back(currentPool);
		}

		bool succeeded = out_set->Create(currentPool, setLayout);

		// Try a second time
		// TODO - In reality, we only ever need to retry if the allocation failed because either the pool ran out of memory,
		//        or it's fragmented. This is represented by VK_ERROR_FRAGMENTED_POOL and VK_ERROR_OUT_OF_POOL_MEMORY error codes
		if (!succeeded)
		{
			// Grab another pool, regardless of the pool we had before, and retry
			currentPool = PickPool();
			usedPools.push_back(currentPool);

			succeeded = out_set->Create(currentPool, setLayout);

			// If we failed again, we can't recover from the issue
			if(!succeeded) return false;
		}

		return true;
	}

	void DescriptorAllocator::DestroyPools()
	{
		VkDevice logicalDevice = GetLogicalDevice();

		for (auto pool : freePools)
		{
			vkDestroyDescriptorPool(logicalDevice, pool.GetPool(), nullptr);
		}

		for (auto pool : usedPools)
		{
			vkDestroyDescriptorPool(logicalDevice, pool.GetPool(), nullptr);
		}

		freePools.clear();
		usedPools.clear();
	}

	DescriptorPool DescriptorAllocator::CreatePool(const DescriptorAllocator::PoolSizes& poolSizes, uint32_t maxSets, VkDescriptorPoolCreateFlags flags)
	{
		std::vector<VkDescriptorPoolSize> sizes;
		sizes.reserve(poolSizes.sizes.size());
		for (auto size : poolSizes.sizes)
		{
			sizes.push_back({ size.first, static_cast<uint32_t>(size.second * maxSets)});
		}

		DescriptorPool descPool;
		descPool.Create(sizes.data(), static_cast<uint32_t>(sizes.size()), maxSets, flags);

		return descPool;
	}

	void DescriptorAllocator::ResetPools()
	{
		for (auto pool : usedPools)
		{
			pool.Reset();
			freePools.push_back(pool);
		}

		usedPools.clear();
		currentPool.ClearHandle();
	}

	DescriptorPool DescriptorAllocator::PickPool()
	{
		if (freePools.size() > 0)
		{
			// Grab pool from the back of the vector and remove it
			DescriptorPool pool = freePools.back();
			freePools.pop_back();
			return pool;
		}
		else
		{
			// No pools available, create a new one
			return CreatePool(descriptorSizes, 100, 0);
		}
	}
}