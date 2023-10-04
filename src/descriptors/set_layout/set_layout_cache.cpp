
#include <algorithm>

#include "set_layout_cache.h"
#include "../../utils/logger.h"
#include "../../utils/sanity_check.h"

namespace TANG
{

	SetLayoutCache::SetLayoutCache() : layoutCache()
	{
		// Nothing to do here
	}

	SetLayoutCache::~SetLayoutCache()
	{
		if (layoutCache.size() != 0)
		{
			LogWarning("Descriptor set layout cache was destructed, but the cache is not empty!");
		}

		layoutCache.clear();
	}

	SetLayoutCache::SetLayoutCache(SetLayoutCache&& other) noexcept : layoutCache(std::move(other.layoutCache))
	{
		// Nothing to do here
	}

	VkDescriptorSetLayout SetLayoutCache::CreateSetLayout(VkDevice logicalDevice, SetLayoutSummary& layoutSummary, VkDescriptorSetLayoutCreateFlags flags)
	{
		if (!layoutSummary.IsValid())
		{
			LogError("Cannot create set layout with an invalid builder!");
			return VK_NULL_HANDLE;
		}

		// Attempt to grab the layout from the cache
		auto iter = layoutCache.find(layoutSummary);
		if (iter != layoutCache.end())
		{
			return iter->second.GetLayout();
		}

		// We didn't find a descriptor set layout matching the description, so let's make a new one
		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.flags = flags;
		createInfo.pBindings = layoutSummary.GetBindings();
		createInfo.bindingCount = layoutSummary.GetBindingCount();

		DescriptorSetLayout layout;
		layout.Create(logicalDevice, createInfo);
		
		layoutCache[layoutSummary] = layout;

		return layout.GetLayout();
	}

	void SetLayoutCache::DestroyLayouts(VkDevice logicalDevice)
	{
		for (auto iter : layoutCache)
		{
			iter.second.Destroy(logicalDevice);
		}

		layoutCache.clear();
	}

	LayoutCache& SetLayoutCache::GetLayoutCache()
	{
		return layoutCache;
	}

	DescriptorSetLayout* SetLayoutCache::GetSetLayout(const SetLayoutSummary& summary)
	{
		auto result = layoutCache.find(summary);
		return result == layoutCache.end() ? nullptr : &(result->second);
	}
}