#ifndef SET_LAYOUT_CACHE_H
#define SET_LAYOUT_CACHE_H

#include <vector>
#include <unordered_map>

#include "set_layout.h"
#include "vulkan/vulkan.h"

namespace TANG
{
	// An allocator and cache class for tracking all the allocated descriptor set layouts
	class SetLayoutCache
	{
	public:

		SetLayoutCache();
		~SetLayoutCache();
		SetLayoutCache(const SetLayoutCache& other) = delete;
		SetLayoutCache(SetLayoutCache&& other) noexcept;
		SetLayoutCache& operator=(const SetLayoutCache& other) = delete;

		// TODO - Create a SetLayoutBuilder class that contains a vector of bindings, so that we can easily create a set layout
		//        by chaining a bunch of AddBinding calls and then pass it into the CreateSetLayout function below
		DescriptorSetLayout CreateSetLayout(VkDevice logicalDevice, VkDescriptorSetLayoutCreateInfo* createInfo);
		void DestroyLayouts(VkDevice logicalDevice);

		struct SetLayoutInfo {

			std::vector<VkDescriptorSetLayoutBinding> bindings;

			bool operator==(const SetLayoutInfo& other) const;

			size_t Hash() const;
		};

	private:

		struct SetLayoutHash {

			std::size_t operator()(const SetLayoutInfo& layoutInfo) const {
				return layoutInfo.Hash();
			}
		};

		std::unordered_map<SetLayoutInfo, DescriptorSetLayout, SetLayoutHash> layoutCache;

	};

}

#endif