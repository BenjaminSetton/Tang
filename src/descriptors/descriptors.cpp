
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
			LogError("Descriptor set layout destructor called but memory was not freed! Memory will be leaked");
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

	void DescriptorSetLayout::AddBinding(VkDevice logicalDevice, uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags)
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
		writeDescriptorSets.clear();
	}

	//WriteDescriptorSets::WriteDescriptorSets(const WriteDescriptorSets& other)
	//{
	//	writeDescriptorSets = other.writeDescriptorSets;
	//}

	WriteDescriptorSets::WriteDescriptorSets(WriteDescriptorSets&& other)
	{
		writeDescriptorSets = std::move(other.writeDescriptorSets);

		other.writeDescriptorSets.clear();
	}

	//WriteDescriptorSets& WriteDescriptorSets::operator=(const WriteDescriptorSets& other)
	//{
	//	writeDescriptorSets = other.writeDescriptorSets;
	//}

	void WriteDescriptorSets::AddUniformBuffer(VkDescriptorSet descriptorSet, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize)
	{
		VkDescriptorBufferInfo bufferInfo{};
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

	void WriteDescriptorSets::AddColorImage(VkDescriptorSet descriptorSet, uint32_t binding, VkImageView imageView, VkSampler sampler)
	{
		VkDescriptorImageInfo imageInfo{};
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


	DescriptorSets::DescriptorSets()
	{
		// Nothing to do here
	}

	DescriptorSets::~DescriptorSets()
	{
		// Nothing to do here
	}

	DescriptorSets::DescriptorSets(const DescriptorSets& other)
	{
		descriptorSets = other.descriptorSets;
	}

	DescriptorSets::DescriptorSets(DescriptorSets&& other) noexcept
	{
		descriptorSets = other.descriptorSets;

		other.descriptorSets.clear();
	}

	DescriptorSets& DescriptorSets::operator=(const DescriptorSets& other)
	{
		// Protect against self-assignment
		if (this == &other)
		{
			return *this;
		}

		descriptorSets = other.descriptorSets;

		return *this;
	}

	void DescriptorSets::Create(VkDevice logicalDevice, DescriptorPool descriptorPool, DescriptorSetLayout setLayout, uint32_t descriptorSetCount)
	{
		std::vector<VkDescriptorSetLayout> setLayouts(descriptorSetCount, setLayout.GetLayout());

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool.GetPool();
		allocInfo.descriptorSetCount = descriptorSetCount;
		allocInfo.pSetLayouts = setLayouts.data();

		descriptorSets.resize(descriptorSetCount);
		if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to allocate descriptor sets!");
		}
	}

	void DescriptorSets::Update(VkDevice logicalDevice, WriteDescriptorSets& writeDescriptorSets)
	{
		uint32_t numWriteDescriptorSets = writeDescriptorSets.GetWriteDescriptorSetCount();
		TNG_ASSERT_MSG(descriptorSets.size() == numWriteDescriptorSets, "Size mismatch between descriptor sets and write descriptor sets!");

		for (uint32_t i = 0; i < descriptorSets.size(); i++)
		{
			vkUpdateDescriptorSets(logicalDevice, writeDescriptorSets.GetWriteDescriptorSetCount(), writeDescriptorSets.GetWriteDescriptorSets(), 0, nullptr);
		}
	}

	void DescriptorSets::Destroy(VkDevice logicalDevice, DescriptorSetLayout setLayout)
	{
		vkDestroyDescriptorSetLayout(logicalDevice, setLayout.GetLayout(), nullptr);
	}

	VkDescriptorSet DescriptorSets::GetDescriptorSet(uint32_t index) const
	{
		if (index < 0 || index >= descriptorSets.size())
		{
			LogError("Invalid descriptor set index");
			return VK_NULL_HANDLE;
		}

		return descriptorSets[index];
	}

	const std::vector<VkDescriptorSet>& DescriptorSets::GetDescriptorSets() const
	{
		return descriptorSets;
	}

}