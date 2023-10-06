
#include <limits>

#include "set_layout_summary.h"
#include "../../utils/logger.h"

static constexpr uint32_t MAX_BINDINGS = 25;
static constexpr uint32_t INVALID_BINDING = std::numeric_limits<uint32_t>::max();

namespace TANG
{

	SetLayoutSummary::SetLayoutSummary() : bindingCount(0)
	{
		bindings.resize(MAX_BINDINGS);
		for (uint32_t i = 0; i < MAX_BINDINGS; i++)
		{
			bindings[i].binding = INVALID_BINDING;
		}
	}

	SetLayoutSummary::~SetLayoutSummary()
	{
		// Nothing to do here
	}

	SetLayoutSummary::SetLayoutSummary(const SetLayoutSummary& other) : bindings(other.bindings), bindingCount(other.bindingCount)
	{
		// Nothing to do here
	}

	SetLayoutSummary::SetLayoutSummary(SetLayoutSummary&& other) : bindings(std::move(other.bindings)), bindingCount(std::move(other.bindingCount))
	{
		// Does std::move() of an integral type take care of this already?
		//other.bindingCount = 0;
	}

	SetLayoutSummary& SetLayoutSummary::operator=(const SetLayoutSummary& other)
	{
		// Protect against self-assignment
		if (this == &other)
		{
			return *this;
		}

		bindings = other.bindings;
		bindingCount = other.bindingCount;

		return *this;
	}

	bool SetLayoutSummary::operator==(const SetLayoutSummary& other) const
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

	void SetLayoutSummary::AddBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags)
	{
		if (bindingCount > MAX_BINDINGS)
		{
			LogError("Exceeded maximum of %i bindings per set layout!", MAX_BINDINGS);
			return;
		}

		if (bindings[binding].binding != INVALID_BINDING)
		{
			LogError("Binding %u already in use! Failed to add new binding for set layout", binding);
			return;
		}

		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = binding;
		layoutBinding.descriptorType = descriptorType;
		layoutBinding.descriptorCount = 1;
		layoutBinding.stageFlags = stageFlags;
		layoutBinding.pImmutableSamplers = nullptr; // Optional

		bindings[binding] = layoutBinding;

		bindingCount++;
	}

	VkDescriptorSetLayoutBinding* SetLayoutSummary::GetBindings()
	{
		return bindings.data();
	}

	uint32_t SetLayoutSummary::GetBindingCount()
	{
		return bindingCount;
	}

	bool SetLayoutSummary::IsValid()
	{
		// The vector is full, so we know it must be contiguous if it passed the sanity checks in AddBinding()
		if (bindingCount == MAX_BINDINGS) return true;

		uint32_t count;
		for (count = 0; count < MAX_BINDINGS; count++)
		{
			if (bindings[count].binding == INVALID_BINDING)
			{
				break;
			}
		}

		return count == bindingCount;
	}

	size_t SetLayoutSummary::Hash() const
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