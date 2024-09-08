#ifndef DESCRIPTOR_SET_H
#define DESCRIPTOR_SET_H

#include <vector>

#include "../utils/sanity_check.h"
#include "vulkan/vulkan.h"

namespace TANG
{
	// Forward declaration
	class DescriptorPool;
	class DescriptorSetLayout;
	class WriteDescriptorSets;

	class DescriptorSet
	{
	public:

		DescriptorSet();
		~DescriptorSet();
		DescriptorSet(const DescriptorSet& other);
		DescriptorSet(DescriptorSet&& other) noexcept;
		DescriptorSet& operator=(const DescriptorSet& other);

		bool Create(const DescriptorPool& descriptorPool, const DescriptorSetLayout& setLayouts);

		void Update(const WriteDescriptorSets& writeDescriptorSets);

		VkDescriptorSet GetDescriptorSet() const;

	private:

		VkDescriptorSet descriptorSet;

	};

	// The size of a DescriptorSet object is guaranteed to be equivalent to that of a VkDescriptorSet object, meaning an array
	// of DescriptorSet objects can be interpreted as an array of VkDescriptorSet objects
	// Guarantee that the size of DescriptorSet and VkDescriptorSet matches
	TNG_ASSERT_SAME_SIZE(DescriptorSet, VkDescriptorSet);
}

#endif