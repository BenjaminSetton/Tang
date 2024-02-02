
#include <algorithm>

#include "../../device_cache.h"
#include "../../utils/logger.h"
#include "../../utils/sanity_check.h"
#include "set_layout_cache.h"

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

	VkDescriptorSetLayout SetLayoutCache::CreateSetLayout(SetLayoutSummary& layoutSummary, VkDescriptorSetLayoutCreateFlags flags)
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
		
		layoutCache.insert({ layoutSummary, DescriptorSetLayout() });

		DescriptorSetLayout& layout = layoutCache.at(layoutSummary);
		layout.Create(createInfo);

		return layout.GetLayout();
	}

	void SetLayoutCache::DestroyLayouts()
	{
		for (auto iter : layoutCache)
		{
			iter.second.Destroy();
		}

		layoutCache.clear();
	}

	std::optional<DescriptorSetLayout> SetLayoutCache::GetSetLayout(uint32_t setNumber) const
	{
		// This is not really efficient, but it does the trick for now
		// TODO - Optimize
		for (const auto& iter : layoutCache)
		{
			if (iter.first.GetSet() == setNumber)
			{
				return iter.second;
			}
		}

		return {};
	}

	std::optional<DescriptorSetLayout> SetLayoutCache::GetSetLayout(const SetLayoutSummary& summary) const
	{
		auto result = layoutCache.find(summary);

		if (result == layoutCache.end())
		{
			return {};
		}
		else
		{
			return result->second;
		}
	}

	uint32_t SetLayoutCache::GetLayoutCount() const
	{
		return static_cast<uint32_t>(layoutCache.size());
	}

	void SetLayoutCache::FlattenCache(std::vector<VkDescriptorSetLayout>& out_setLayoutArray) const
	{
		for (uint32_t i = 0; i < GetLayoutCount(); i++)
		{
			out_setLayoutArray.push_back(GetSetLayout(i).value().GetLayout());
		}
	}
}