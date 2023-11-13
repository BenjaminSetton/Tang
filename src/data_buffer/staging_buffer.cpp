
#include <cstring> // memcpy
#include <utility>

#include "staging_buffer.h"
#include "../utils/logger.h"

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

	void StagingBuffer::Create(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkDeviceSize size)
	{
		CreateBase(physicalDevice, logicalDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}

	void StagingBuffer::Destroy(VkDevice logicalDevice)
	{
		if(buffer) vkDestroyBuffer(logicalDevice, buffer, nullptr);
		if(bufferMemory) vkFreeMemory(logicalDevice, bufferMemory, nullptr);

		buffer = VK_NULL_HANDLE;
		bufferMemory = VK_NULL_HANDLE;

		bufferState = BUFFER_STATE::DESTROYED;
	}

	void StagingBuffer::CopyIntoBuffer(VkDevice logicalDevice, void* sourceData, VkDeviceSize size)
	{
		if (IsInvalid())
		{
			LogWarning("Attempting to copy into invalid staging buffer!");
			return;
		}

		void* bufferPtr;
		VkResult res = vkMapMemory(logicalDevice, bufferMemory, 0, size, 0, &bufferPtr);
		if (res != VK_SUCCESS)
		{
			LogError("Failed to map memory for staging buffer!");
		}
		memcpy(bufferPtr, sourceData, size);
		vkUnmapMemory(logicalDevice, bufferMemory);
	}

}