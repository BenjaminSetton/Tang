
#include "set_layout.h"

#include "../../utils/logger.h"
#include "../../utils/sanity_check.h"

namespace TANG
{
	TNG_ASSERT_SAME_SIZE(sizeof(DescriptorSetLayout), sizeof(VkDescriptorSetLayout));

	DescriptorSetLayout::DescriptorSetLayout()
	{
		// Nothing to do here
	}

	DescriptorSetLayout::~DescriptorSetLayout()
	{
		if (setLayout != VK_NULL_HANDLE)
		{
			LogError("Descriptor set layout destructor called but memory was not freed! Memory will be leaked");
		}

		setLayout = VK_NULL_HANDLE;
	}

	DescriptorSetLayout::DescriptorSetLayout(const DescriptorSetLayout& other) : setLayout(other.setLayout)
	{
		// Nothing to do here
	}

	DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) noexcept : setLayout(other.setLayout)
	{
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

		return *this;
	}

	//void DescriptorSetLayout::AddBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags)
	//{
	//	if (bindings.find(binding) != bindings.end())
	//	{
	//		LogError("Binding %u already in use! Failed to add new binding for descriptor set layout", binding);
	//		return;
	//	}

	//	VkDescriptorSetLayoutBinding layoutBinding{};
	//	layoutBinding.binding = binding;
	//	layoutBinding.descriptorType = descriptorType;
	//	layoutBinding.descriptorCount = 1;
	//	layoutBinding.stageFlags = stageFlags;
	//	layoutBinding.pImmutableSamplers = nullptr; // Optional

	//	bindings.insert({ binding, layoutBinding });
	//}

	void DescriptorSetLayout::Create(VkDevice logicalDevice, VkDescriptorSetLayoutCreateInfo& createInfo)
	{
		if (setLayout != VK_NULL_HANDLE)
		{
			LogWarning("Overwriting descriptor set layout");
		}

		if (vkCreateDescriptorSetLayout(logicalDevice, &createInfo, nullptr, &setLayout) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create descriptor set layout!");
		}
	}

	void DescriptorSetLayout::Destroy(VkDevice logicalDevice)
	{
		if (setLayout == VK_NULL_HANDLE)
		{
			LogError("Descriptor set layout is already destroyed, but we're attempting to destroy it again");
			return;
		}

		vkDestroyDescriptorSetLayout(logicalDevice, setLayout, nullptr);
		setLayout = VK_NULL_HANDLE;
	}

	VkDescriptorSetLayout& DescriptorSetLayout::GetLayout()
	{
		return setLayout;
	}
}