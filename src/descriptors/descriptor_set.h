#ifndef DESCRIPTOR_SET_H
#define DESCRIPTOR_SET_H

#include <unordered_map>
#include <vector>

#include "vulkan/vulkan.h"

namespace TANG
{
	// Forward declaration
	class DescriptorPool;
	class DescriptorSetLayout;
	class WriteDescriptorSets;

	// All the components above must be used in one way or another to create and update the descriptor sets.
	// The size of a DescriptorSet object is equivalent to that of a VkDescriptorSet object, meaning an array
	// of DescriptorSet objects can be interpreted as an array of VkDescriptorSet objects
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
}

#endif