#ifndef SET_LAYOUT_CACHE_H
#define SET_LAYOUT_CACHE_H

#include <vector>
#include <unordered_map>

#include "set_layout_summary.h"
#include "set_layout.h"
#include "vulkan/vulkan.h"

namespace TANG
{
	// PRIVATE - DO NOT USE
	// This is only here so we can define a convenient typedef for the cache type below
	struct SetLayoutHash {

		size_t operator()(const SetLayoutSummary& layoutSummary) const {
			return layoutSummary.Hash();
		}
	};

	typedef std::unordered_map<SetLayoutSummary, DescriptorSetLayout, SetLayoutHash> LayoutCache;

	// An allocator and cache class for tracking all the allocated descriptor set layouts
	class SetLayoutCache
	{
	public:

		SetLayoutCache();
		~SetLayoutCache();
		SetLayoutCache(const SetLayoutCache& other) = delete;
		SetLayoutCache(SetLayoutCache&& other) noexcept;
		SetLayoutCache& operator=(const SetLayoutCache& other) = delete;

		VkDescriptorSetLayout CreateSetLayout(VkDevice logicalDevice, SetLayoutSummary& layoutSummary, VkDescriptorSetLayoutCreateFlags flags);
		void DestroyLayouts(VkDevice logicalDevice);

		const LayoutCache& GetLayoutCache() const;
		DescriptorSetLayout* GetSetLayout(const SetLayoutSummary& summary);
		uint32_t GetLayoutCount() const;

	private:

		LayoutCache layoutCache;

	};

}

#endif