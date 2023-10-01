
#include "descriptors.h"

#include "descriptor_pool.h"
#include "../utils/logger.h"
#include "../utils/sanity_check.h"

namespace TANG
{
	/////////////////////////////////////////////////////////////////////////////////////////
	//
	//		DESCRIPTOR SET LAYOUT
	//
	/////////////////////////////////////////////////////////////////////////////////////////

	DescriptorSetLayout::DescriptorSetLayout() : state(SET_LAYOUT_STATE::DEFAULT)
	{
		// Nothing to do here
	}

	DescriptorSetLayout::~DescriptorSetLayout()
	{
		if (state != SET_LAYOUT_STATE::DESTROYED)
		{
			LogError("Descriptor set layout destructor called but memory was not freed! Memory could be leaked");
		}

		setLayout = VK_NULL_HANDLE;
		bindings.clear();
	}

	DescriptorSetLayout::DescriptorSetLayout(const DescriptorSetLayout& other) : setLayout(other.setLayout), bindings(other.bindings), state(other.state)
	{
		// Nothing to do here
	}

	DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) : setLayout(other.setLayout), bindings(std::move(other.bindings)), state(other.state)
	{
		other.bindings.clear();
		setLayout = VK_NULL_HANDLE;
	}

	DescriptorSetLayout& DescriptorSetLayout::operator=(const DescriptorSetLayout& other)
	{
		// Protect against self-assignment
		if (this == &other)
		{
			return *this;
		}

		setLayout = other.setLayout;
		bindings = other.bindings;
		state = other.state;

		return *this;
	}

	void DescriptorSetLayout::AddBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags)
	{
		if (bindings.find(binding) != bindings.end())
		{
			LogError("Binding %u already in use! Failed to add new binding for descriptor set layout", binding);
			return;
		}

		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = binding;
		layoutBinding.descriptorType = descriptorType;
		layoutBinding.descriptorCount = 1;
		layoutBinding.stageFlags = stageFlags;
		layoutBinding.pImmutableSamplers = nullptr; // Optional

		bindings.insert({ binding, layoutBinding });
	}

	void DescriptorSetLayout::Create(VkDevice logicalDevice)
	{
		if (state == SET_LAYOUT_STATE::CREATED)
		{
			LogWarning("Overwriting descriptor set layout");
		}
		else if (state == SET_LAYOUT_STATE::DESTROYED)
		{
			LogWarning("Failed to create descriptor set layout, object is already destroyed");
			return;
		}

		// Convert the unordered map into a contiguous array
		std::vector<VkDescriptorSetLayoutBinding> bindingsArray;
		for (auto& iter : bindings)
		{
			bindingsArray.push_back(iter.second);
		}

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.bindingCount = static_cast<uint32_t>(bindingsArray.size());
		createInfo.pBindings = bindingsArray.data();

		if (vkCreateDescriptorSetLayout(logicalDevice, &createInfo, nullptr, &setLayout) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create descriptor set layout!");
		}

		state = SET_LAYOUT_STATE::CREATED;
	}

	void DescriptorSetLayout::Destroy(VkDevice logicalDevice)
	{
		if (state == SET_LAYOUT_STATE::DESTROYED)
		{
			LogError("Descriptor set layout is already destroyed, but we're attempting to destroy it again");
			return;
		}

		vkDestroyDescriptorSetLayout(logicalDevice, setLayout, nullptr);
		setLayout = VK_NULL_HANDLE;

		state = SET_LAYOUT_STATE::DESTROYED;
	}

	VkDescriptorSetLayout& DescriptorSetLayout::GetLayout()
	{
		return setLayout;
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//
	//		WRITE DESCRIPTOR SETS
	//
	/////////////////////////////////////////////////////////////////////////////////////////

	WriteDescriptorSets::WriteDescriptorSets()
	{
		// Nothing to do here
	}

	WriteDescriptorSets::~WriteDescriptorSets()
	{
		descriptorImageInfo.clear();
		descriptorBufferInfo.clear();
		writeDescriptorSets.clear();
	}

	WriteDescriptorSets::WriteDescriptorSets(WriteDescriptorSets&& other) : 
		writeDescriptorSets(std::move(other.writeDescriptorSets)), descriptorBufferInfo(std::move(descriptorBufferInfo)), descriptorImageInfo(std::move(descriptorImageInfo))
	{
		// Nothing to do here
	}

	void WriteDescriptorSets::AddUniformBuffer(VkDescriptorSet descriptorSet, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize)
	{
		descriptorBufferInfo.emplace_back(std::move(VkDescriptorBufferInfo()));
		VkDescriptorBufferInfo& bufferInfo = descriptorBufferInfo.back();
		bufferInfo.buffer = buffer;
		bufferInfo.offset = 0;
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
	}

	void WriteDescriptorSets::AddImageSampler(VkDescriptorSet descriptorSet, uint32_t binding, VkImageView imageView, VkSampler sampler)
	{
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
	}

	uint32_t WriteDescriptorSets::GetWriteDescriptorSetCount() const
	{
		return static_cast<uint32_t>(writeDescriptorSets.size());
	}

	const VkWriteDescriptorSet* WriteDescriptorSets::GetWriteDescriptorSets() const
	{
		return writeDescriptorSets.data();
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//
	//		DESCRIPTOR SETS
	//
	/////////////////////////////////////////////////////////////////////////////////////////

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

	/////////////////////////////////////////////////////////////////////////////////////////
	//
	//		DESCRIPTOR BUNDLE
	//
	/////////////////////////////////////////////////////////////////////////////////////////

	DescriptorBundle::DescriptorBundle() : descSet(DescriptorSet()), setLayout(DescriptorSetLayout())
	{
		// Nothing to do here
	}

	DescriptorBundle::~DescriptorBundle()
	{
		// Nothing to do here
	}

	DescriptorBundle::DescriptorBundle(const DescriptorBundle& other) : descSet(other.descSet), setLayout(other.setLayout)
	{
		// Nothing to do here
	}

	DescriptorBundle::DescriptorBundle(DescriptorBundle&& other) : descSet(std::move(other.descSet)), setLayout(std::move(other.setLayout))
	{
		// Remove handles in other object. We need to mark this class as their friend so we can properly remove their handles
		other.descSet.descriptorSet = VK_NULL_HANDLE;
		other.setLayout.setLayout = VK_NULL_HANDLE;
	}

	DescriptorBundle& DescriptorBundle::operator=(const DescriptorBundle& other)
	{
		// Protect against self-assignment
		if (this == &other)
		{
			return *this;
		}

		descSet = other.descSet;
		setLayout = other.setLayout;

		return *this;
	}

	DescriptorSet* DescriptorBundle::GetDescriptorSet()
	{
		return &descSet;
	}

	DescriptorSetLayout* DescriptorBundle::GetDescriptorSetLayout()
	{
		return &setLayout;
	}
}