
#include "set_layout.h"

#include "../../utils/logger.h"
#include "../../utils/sanity_check.h"

namespace TANG
{
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

	DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) noexcept : setLayout(other.setLayout), bindings(std::move(other.bindings)), state(other.state)
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
}