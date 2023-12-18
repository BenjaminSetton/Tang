
#include "../device_cache.h"
#include "../utils/logger.h"
#include "../utils/sanity_check.h"
#include "descriptor_set.h"
#include "descriptor_pool.h"
#include "set_layout/set_layout.h"
#include "write_descriptor_set.h"

namespace TANG
{
	// Guarantee that the size of DescriptorSet and VkDescriptorSet matches
	TNG_ASSERT_SAME_SIZE(DescriptorSet, VkDescriptorSet);

	DescriptorSet::DescriptorSet() : descriptorSet(VK_NULL_HANDLE)
	{
		// Nothing to do here
	}

	DescriptorSet::~DescriptorSet()
	{
		// TODO - Figure out a way to report descriptor sets that were never freed because the pool itself was never freed

		/*if (setState != DESCRIPTOR_SET_STATE::DESTROYED)
		{
			LogWarning("Descriptor set object was destructed, but memory was not freed!");
		}*/

		LogInfo("Destructed descriptor set!");
	}

	DescriptorSet::DescriptorSet(const DescriptorSet& other)
	{
		descriptorSet = other.descriptorSet;

		LogInfo("Copied descriptor set!");
	}

	DescriptorSet::DescriptorSet(DescriptorSet&& other) noexcept
	{
		descriptorSet = other.descriptorSet;

		other.descriptorSet = VK_NULL_HANDLE;

		LogInfo("Moved descriptor set!");
	}

	DescriptorSet& DescriptorSet::operator=(const DescriptorSet& other)
	{
		// Protect against self-assignment
		if (this == &other)
		{
			return *this;
		}

		descriptorSet = other.descriptorSet;

		LogInfo("Copy-assigned descriptor set!");

		return *this;
	}

	bool DescriptorSet::Create(const DescriptorPool& descriptorPool, const DescriptorSetLayout& setLayout)
	{
		if (descriptorSet != VK_NULL_HANDLE)
		{
			LogWarning("Attempted to create the same descriptor set more than once!");
			return false;
		}

		VkDescriptorSetLayout layout[] = { setLayout.GetLayout() };

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool.GetPool();
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = layout;

		if (vkAllocateDescriptorSets(GetLogicalDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS)
		{
			LogError("Failed to allocate descriptor sets!");
			return false;
		}

		return true;
	}

	void DescriptorSet::Update(const WriteDescriptorSets& writeDescriptorSets)
	{
		if (descriptorSet == VK_NULL_HANDLE)
		{
			LogError("Cannot update a descriptor set that has not been created or has already been destroyed! Bailing...");
			return;
		}

		uint32_t numWriteDescriptorSets = writeDescriptorSets.GetWriteDescriptorSetCount();

		vkUpdateDescriptorSets(GetLogicalDevice(), numWriteDescriptorSets, writeDescriptorSets.GetWriteDescriptorSets(), 0, nullptr);
	}

	VkDescriptorSet DescriptorSet::GetDescriptorSet() const
	{
		return descriptorSet;
	}
}