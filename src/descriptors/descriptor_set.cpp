
#include "descriptor_set.h"

#include "descriptor_pool.h"
#include "set_layout/set_layout.h"
#include "write_descriptor_set.h"
#include "../utils/logger.h"
#include "../utils/sanity_check.h"

namespace TANG
{

	DescriptorSet::DescriptorSet() : descriptorSet(VK_NULL_HANDLE), setState(DESCRIPTOR_SET_STATE::DEFAULT)
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
		setState = other.setState;

		LogInfo("Copied descriptor set!");
	}

	DescriptorSet::DescriptorSet(DescriptorSet&& other) noexcept
	{
		descriptorSet = other.descriptorSet;
		setState = other.setState;

		other.descriptorSet = VK_NULL_HANDLE;
		other.setState = DESCRIPTOR_SET_STATE::DEFAULT; // Maybe we should make a MOVED state?

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
		setState = other.setState;

		LogInfo("Copy-assigned descriptor set!");

		return *this;
	}

	void DescriptorSet::Create(VkDevice logicalDevice, DescriptorPool& descriptorPool, DescriptorSetLayout& setLayout)
	{
		if (setState == DESCRIPTOR_SET_STATE::CREATED)
		{
			LogWarning("Attempted to create the same descriptor set more than once!");
			return;
		}

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool.GetPool();
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &setLayout.GetLayout();

		if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSet) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to allocate descriptor sets!");
		}

		setState = DESCRIPTOR_SET_STATE::CREATED;
	}

	void DescriptorSet::Update(VkDevice logicalDevice, WriteDescriptorSets& writeDescriptorSets)
	{
		if (setState != DESCRIPTOR_SET_STATE::CREATED)
		{
			LogError("Cannot update a descriptor set that has not been created or has already been destroyed! Bailing...");
			return;
		}

		uint32_t numWriteDescriptorSets = writeDescriptorSets.GetWriteDescriptorSetCount();

		vkUpdateDescriptorSets(logicalDevice, numWriteDescriptorSets, writeDescriptorSets.GetWriteDescriptorSets(), 0, nullptr);
	}

	VkDescriptorSet DescriptorSet::GetDescriptorSet() const
	{
		return descriptorSet;
	}
}