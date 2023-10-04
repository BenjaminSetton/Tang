
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

		//SetLayoutInfo layoutInfo{};
		//layoutInfo.bindings.reserve(builder.GetBindingCount());
		//bool isSorted = true;
		//int lastBinding = -1;

		//// Copy the bindings from the create info to our internal info struct
		//for (uint32_t i = 0; i < builder.GetBindingCount(); i++)
		//{
		//	layoutInfo.bindings.push_back(createInfo->pBindings[i]);

		//	// Check that the bindings are in ascending order. This is necessary when hashing
		//	// If the current binding is smaller or equal to the last binding, then we'll say it's NOT sorted
		//	int currentBinding = static_cast<int>(createInfo->pBindings[i].binding);
		//	isSorted &= (currentBinding <= lastBinding);

		//	lastBinding = currentBinding;
		//}

		//// Sort the bindings if they aren't already
		//if (!isSorted)
		//{
		//	std::sort(layoutInfo.bindings.begin(), layoutInfo.bindings.end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b) {
		//		return a.binding < b.binding;
		//	});
		//}

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