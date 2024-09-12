#ifndef SET_LAYOUT_H
#define SET_LAYOUT_H

#include "../../utils/sanity_check.h"
#include "vulkan/vulkan.h"

namespace TANG
{
	// Encapsulates a descriptor set layout
	class DescriptorSetLayout
	{
	public:

		DescriptorSetLayout();
		~DescriptorSetLayout();
		DescriptorSetLayout(const DescriptorSetLayout& other);
		DescriptorSetLayout(DescriptorSetLayout&& other) noexcept;
		DescriptorSetLayout& operator=(const DescriptorSetLayout& other);

		void Create(VkDescriptorSetLayoutCreateInfo& createInfo);
		void Destroy();

		VkDescriptorSetLayout GetLayout() const;

	private:

		VkDescriptorSetLayout setLayout;
	};

	// The size is guaranteed to be the same as the underlying VkDescriptorSetLayout object
	TNG_ASSERT_SAME_SIZE(DescriptorSetLayout, VkDescriptorSetLayout);
}

#endif