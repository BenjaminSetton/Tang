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

	// All the components above must be used in one way or another to create and update the descriptor sets
	class DescriptorSet
	{
	public:

		enum class DESCRIPTOR_SET_STATE
		{
			DEFAULT,
			CREATED,
			DESTROYED
		};

		DescriptorSet();
		~DescriptorSet();
		DescriptorSet(const DescriptorSet& other);
		DescriptorSet(DescriptorSet&& other) noexcept;
		DescriptorSet& operator=(const DescriptorSet& other);

		void Create(VkDevice logicalDevice, DescriptorPool& descriptorPool, DescriptorSetLayout& setLayouts);

		void Update(VkDevice logicalDevice, WriteDescriptorSets& writeDescriptorSets);

		VkDescriptorSet GetDescriptorSet() const;

	private:

		VkDescriptorSet descriptorSet;
		DESCRIPTOR_SET_STATE setState;

	};
}

#endif