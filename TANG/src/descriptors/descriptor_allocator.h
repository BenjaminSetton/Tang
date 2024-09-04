#ifndef DESCRIPTOR_ALLOCATOR_H
#define DESCRIPTOR_ALLOCATOR_H

#include <vector>

#include "descriptor_pool.h"
#include "vulkan/vulkan.h"

namespace TANG
{
	// Forward declarations
	class DescriptorSet;
	class DescriptorSetLayout;

	class DescriptorAllocator
	{
	public:

		DescriptorAllocator();
		~DescriptorAllocator();
		DescriptorAllocator(const DescriptorAllocator& other);
		DescriptorAllocator(DescriptorAllocator&& other) noexcept;
		DescriptorAllocator& operator=(const DescriptorAllocator& other);

		struct PoolSizes {
			std::vector<std::pair<VkDescriptorType, float>> sizes =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
			};
		};

		bool CreateSet(DescriptorSetLayout setLayout, DescriptorSet* set);

		// Creates a pool given the pool sizes, max sets and flags
		DescriptorPool CreatePool(const DescriptorAllocator::PoolSizes& poolSizes, uint32_t maxSets, VkDescriptorPoolCreateFlags flags);
		void DestroyPools();
		void ResetPools();

	private:

		DescriptorPool PickPool();

		DescriptorPool currentPool;
		PoolSizes descriptorSizes;
		std::vector<DescriptorPool> usedPools;
		std::vector<DescriptorPool> freePools;

	};
}


#endif