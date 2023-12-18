
#include "set_layout.h"

#include "../../device_cache.h"
#include "../../utils/logger.h"
#include "../../utils/sanity_check.h"

namespace TANG
{
	TNG_ASSERT_SAME_SIZE(sizeof(DescriptorSetLayout), sizeof(VkDescriptorSetLayout));

	DescriptorSetLayout::DescriptorSetLayout() : setLayout(VK_NULL_HANDLE)
	{
		// Nothing to do here
	}

	DescriptorSetLayout::~DescriptorSetLayout()
	{
		if (setLayout != VK_NULL_HANDLE)
		{
			// TODO - Detect if the layout has been copied or moved so we can avoid warning in those cases
			LogWarning("Descriptor set layout destructor called but memory was not freed! Memory will be leaked");
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

	void DescriptorSetLayout::Create(VkDescriptorSetLayoutCreateInfo& createInfo)
	{
		VkDevice logicalDevice = GetLogicalDevice();

		if (setLayout != VK_NULL_HANDLE)
		{
			LogWarning("Overwriting descriptor set layout");
		}

		if (vkCreateDescriptorSetLayout(logicalDevice, &createInfo, nullptr, &setLayout) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create descriptor set layout!");
		}
	}

	void DescriptorSetLayout::Destroy()
	{
		VkDevice logicalDevice = GetLogicalDevice();

		if (setLayout == VK_NULL_HANDLE)
		{
			LogError("Descriptor set layout is already destroyed, but we're attempting to destroy it again");
			return;
		}

		vkDestroyDescriptorSetLayout(logicalDevice, setLayout, nullptr);
		setLayout = VK_NULL_HANDLE;
	}

	VkDescriptorSetLayout DescriptorSetLayout::GetLayout() const
	{
		return setLayout;
	}
}