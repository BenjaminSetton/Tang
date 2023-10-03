#ifndef SET_LAYOUT_H
#define SET_LAYOUT_H

#include <unordered_map>

#include "vulkan/vulkan.h"

namespace TANG
{
	// Encapsulates a descriptor set layout, as well as the functionality for adding bindings and creating the descriptor set layout itself
	// Note that it's allowed to copy this object. This is because we must use one set layout for every descriptor set, but we may only
	// define one set layout and decide to reuse it for every descriptor set
	class DescriptorSetLayout
	{
	public:

		enum class SET_LAYOUT_STATE
		{
			DEFAULT,
			CREATED,
			DESTROYED
		};

		DescriptorSetLayout();
		~DescriptorSetLayout();
		DescriptorSetLayout(const DescriptorSetLayout& other);
		DescriptorSetLayout(DescriptorSetLayout&& other) noexcept;
		DescriptorSetLayout& operator=(const DescriptorSetLayout& other);

		void AddBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags);

		void Create(VkDevice logicalDevice);
		void Destroy(VkDevice logicalDevice);

		VkDescriptorSetLayout& GetLayout();

	private:

		VkDescriptorSetLayout setLayout;
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
		SET_LAYOUT_STATE state;
	};
}

#endif