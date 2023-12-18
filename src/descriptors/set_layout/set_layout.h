#ifndef SET_LAYOUT_H
#define SET_LAYOUT_H

#include "vulkan/vulkan.h"

namespace TANG
{
	// Encapsulates a descriptor set layout, and the size is guaranteed to be the same as the underlying VkDescriptorSetLayout object
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
}

#endif