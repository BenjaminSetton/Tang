#ifndef SET_LAYOUT_CACHE_H
#define SET_LAYOUT_CACHE_H

#include <unordered_map>
#include <vector>

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

		void CreateSetLayout(SetLayoutSummary& layoutSummary, VkDescriptorSetLayoutCreateFlags flags);
		void DestroyLayouts();

		const DescriptorSetLayout* GetSetLayout(uint32_t setNumber) const;
		const DescriptorSetLayout* GetSetLayout(const SetLayoutSummary& summary) const;
		uint32_t GetLayoutCount() const;

		void FlattenCache(std::vector<VkDescriptorSetLayout>& out_setLayoutArray) const;

	private:

		LayoutCache layoutCache;

	};

}

#endif