#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H

#include "vulkan/vulkan.h"

#include <unordered_map>
#include <vector>

namespace TANG
{
	// Forward declaration
	class DescriptorPool;


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
		DescriptorSetLayout(DescriptorSetLayout&& other);
		DescriptorSetLayout& operator=(const DescriptorSetLayout& other);

		void AddBinding(VkDevice logicalDevice, uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags);

		void Create(VkDevice logicalDevice);
		void Destroy(VkDevice logicalDevice);

		VkDescriptorSetLayout& GetLayout();

	private:

		VkDescriptorSetLayout setLayout;
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
		SET_LAYOUT_STATE state;
	};


	// Encapsulates the data and functionality for creating a WriteDescriptorSet
	// Note that it's not allowed to copy an object of this class, it should only be moved
	class WriteDescriptorSets
	{
	public:

		WriteDescriptorSets();
		~WriteDescriptorSets();
		WriteDescriptorSets(const WriteDescriptorSets& other) = delete;
		WriteDescriptorSets(WriteDescriptorSets&& other);
		WriteDescriptorSets& operator=(const WriteDescriptorSets& other) = delete;

		void AddUniformBuffer(VkDescriptorSet descriptorSet, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize);
		void AddColorImage(VkDescriptorSet descriptorSet, uint32_t binding, VkImageView imageView, VkSampler sampler);

		uint32_t GetWriteDescriptorSetCount() const;
		const VkWriteDescriptorSet* GetWriteDescriptorSets() const;

	private:

		std::vector<VkWriteDescriptorSet> writeDescriptorSets;

	};

	// All the components above must be used in one way or another to create and update the descriptor sets
	class DescriptorSets
	{
	public:

		DescriptorSets();
		~DescriptorSets();
		DescriptorSets(const DescriptorSets& other);
		DescriptorSets(DescriptorSets&& other) noexcept;
		DescriptorSets& operator=(const DescriptorSets& other);

		void Create(VkDevice logicalDevice, DescriptorPool descriptorPool, DescriptorSetLayout setLayouts, uint32_t descriptorSetCount);

		void Update(VkDevice logicalDevice, WriteDescriptorSets& writeDescriptorSets);

		void Destroy(VkDevice logicalDevice, DescriptorSetLayout setLayout);

		VkDescriptorSet GetDescriptorSet(uint32_t index) const;
		uint32_t GetDescriptorSetCount() const;

		std::vector<VkDescriptorSet>& GetDescriptorSets();

	private:

		std::vector<VkDescriptorSet> descriptorSets;

		// TODO - IMPLEMENT THIS
		std::vector<VkDescriptorSetLayout> setLayouts;

	};
}

#endif