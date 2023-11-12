
#include <utility>

#include "staging_buffer.h"

namespace TANG
{

	StagingBuffer::StagingBuffer()
	{
	}

	StagingBuffer::~StagingBuffer()
	{
	}

	StagingBuffer::StagingBuffer(StagingBuffer&& other) noexcept : Buffer(std::move(other))
	{
	}

	void StagingBuffer::Create(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkDeviceSize size)
	{
		CreateBase(physicalDevice, logicalDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}

	void StagingBuffer::Destroy(VkDevice& logicalDevice)
	{
		if(buffer) vkDestroyBuffer(logicalDevice, buffer, nullptr);
		if(bufferMemory) vkFreeMemory(logicalDevice, bufferMemory, nullptr);

		buffer = VK_NULL_HANDLE;
		bufferMemory = VK_NULL_HANDLE;

		bufferState = BUFFER_STATE::DESTROYED;
	}

}