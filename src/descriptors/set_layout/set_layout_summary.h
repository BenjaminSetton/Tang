#ifndef SET_SUMMARY_H
#define SET_SUMMARY_H

#include <vector>

#include "vulkan/vulkan.h"

namespace TANG
{
	// A summary of a descriptor set layout
	class SetLayoutSummary
	{
	public:

		explicit SetLayoutSummary(uint32_t setNumber);
		~SetLayoutSummary();
		SetLayoutSummary(const SetLayoutSummary& other);
		SetLayoutSummary(SetLayoutSummary&& other);
		SetLayoutSummary& operator=(const SetLayoutSummary& other);
		bool operator==(const SetLayoutSummary& other) const;

		void AddBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags);

		VkDescriptorSetLayoutBinding* GetBindings();
		uint32_t GetBindingCount();
		uint32_t GetSet() const;

		// Returns true if the internal binding vector is contiguous, false otherwise
		bool IsValid();

		// Hashes the bindings and returns the result. Everything in a vulkan set layout binding objects is considered, except
		// for the pImmutableSamplers
		size_t Hash() const;

	private:

		std::vector<VkDescriptorSetLayoutBinding> bindings;
		uint32_t bindingCount;
		uint32_t set;

	};
}

#endif