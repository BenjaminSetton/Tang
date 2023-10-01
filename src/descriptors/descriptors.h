#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H

#include "vulkan/vulkan.h"

#include <unordered_map>
#include <vector>

namespace TANG
{
	// Forward declaration
	class DescriptorPool;
	class DescriptorBundle;


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

		void AddBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags);

		void Create(VkDevice logicalDevice);
		void Destroy(VkDevice logicalDevice);

		VkDescriptorSetLayout& GetLayout();

	private:

		friend class DescriptorBundle;

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
		void AddImageSampler(VkDescriptorSet descriptorSet, uint32_t binding, VkImageView imageView, VkSampler sampler);

		uint32_t GetWriteDescriptorSetCount() const;
		const VkWriteDescriptorSet* GetWriteDescriptorSets() const;

	private:

		std::vector<VkWriteDescriptorSet> writeDescriptorSets;

		// Temporary memory, since a VkWriteDescriptorSet object requires a pointer to these structs
		std::vector<VkDescriptorImageInfo> descriptorImageInfo;
		std::vector<VkDescriptorBufferInfo> descriptorBufferInfo;

	};

	// All the components above must be used in one way or another to create and update the descriptor sets
	class DescriptorSet
	{
	public:

		enum class DESCRIPTOR_SET_STATE
		{
			DEFAULT,
			CREATED,
			DESTROYED
		};

		DescriptorSet();
		~DescriptorSet();
		DescriptorSet(const DescriptorSet& other);
		DescriptorSet(DescriptorSet&& other) noexcept;
		DescriptorSet& operator=(const DescriptorSet& other);

		void Create(VkDevice logicalDevice, DescriptorPool& descriptorPool, DescriptorSetLayout& setLayouts);

		void Update(VkDevice logicalDevice, WriteDescriptorSets& writeDescriptorSets);

		VkDescriptorSet GetDescriptorSet() const;

	private:

		friend class DescriptorBundle;

		VkDescriptorSet descriptorSet;
		DESCRIPTOR_SET_STATE setState;

	};

	// Bundles up a descriptor set and the set layout that describes it
	class DescriptorBundle
	{
	public:

		DescriptorBundle();
		~DescriptorBundle();
		DescriptorBundle(const DescriptorBundle& other);
		DescriptorBundle(DescriptorBundle&& other);
		DescriptorBundle& operator=(const DescriptorBundle& other);

		DescriptorSet* GetDescriptorSet();
		DescriptorSetLayout* GetDescriptorSetLayout();

	private:

		DescriptorSet descSet;
		DescriptorSetLayout setLayout;
	};
}

#endif