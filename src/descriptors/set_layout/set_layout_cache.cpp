
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

	DescriptorSetLayout SetLayoutCache::CreateSetLayout(VkDevice logicalDevice, VkDescriptorSetLayoutCreateInfo* createInfo)
	{
		TNG_ASSERT_MSG(createInfo != nullptr, "Cannot create descriptor set layout when create info is nullptr!");

		SetLayoutInfo layoutInfo{};
		layoutInfo.bindings.reserve(createInfo->bindingCount);
		bool isSorted = true;
		int lastBinding = -1;

		// Copy the bindings from the create info to our internal info struct
		for (uint32_t i = 0; i < createInfo->bindingCount; i++)
		{
			layoutInfo.bindings.push_back(createInfo->pBindings[i]);

			// Check that the bindings are in ascending order. This is necessary when hashing
			// If the current binding is smaller or equal to the last binding, then we'll say it's NOT sorted
			int currentBinding = static_cast<int>(createInfo->pBindings[i].binding);
			isSorted &= (currentBinding <= lastBinding);

			lastBinding = currentBinding;
		}

		// Sort the bindings if they aren't already
		if (!isSorted)
		{
			std::sort(layoutInfo.bindings.begin(), layoutInfo.bindings.end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b) {
				return a.binding < b.binding;
			});
		}

		// Attempt to grab the layout from the cache
		auto iter = layoutCache.find(layoutInfo);
		if (iter != layoutCache.end())
		{
			return iter->second;
		}

		// We didn't find a descriptor set layout matching the description, so let's make a new one
		DescriptorSetLayout layout;
		layout.Create(logicalDevice, *createInfo);
		
		layoutCache[layoutInfo] = layout;
		return layout;
	}

	void SetLayoutCache::DestroyLayouts(VkDevice logicalDevice)
	{
		for (auto iter : layoutCache)
		{
			iter.second.Destroy(logicalDevice);
		}

		layoutCache.clear();
	}

	///////////////////////////////////////////////////////
	//
	//		DESCRIPTOR LAYOUT INFO
	//
	///////////////////////////////////////////////////////

	bool SetLayoutCache::SetLayoutInfo::operator==(const SetLayoutInfo& other) const
	{
		if (bindings.size() != other.bindings.size())
		{
			return false;
		}

		// Check that the bindings are equal. We check for everything EXCEPT pImmutableSamplers
		for (uint32_t i = 0; i < bindings.size(); i++)
		{
			if (bindings[i].binding != other.bindings[i].binding)                 return false;
			if (bindings[i].descriptorType != other.bindings[i].descriptorType)   return false;
			if (bindings[i].descriptorCount != other.bindings[i].descriptorCount) return false;
			if (bindings[i].stageFlags != other.bindings[i].stageFlags)           return false;
		}

		return true;
	}

	size_t SetLayoutCache::SetLayoutInfo::Hash() const
	{
		size_t hashResult = std::hash<size_t>()(bindings.size());

		// I'll admit, I did not come up with this hashing algorithm. Full credit to:
		// https://vkguide.dev/docs/extra-chapter/abstracting_descriptors/
		for (auto& binding : bindings)
		{
			//pack the binding data into a single int64. Not fully correct but it's ok
			size_t bindingHash = binding.binding | binding.descriptorType << 8 | binding.descriptorCount << 16 | binding.stageFlags << 24;

			//shuffle the packed binding data and xor it with the main hash
			hashResult ^= std::hash<size_t>()(bindingHash);
		}

		return hashResult;
	}

}