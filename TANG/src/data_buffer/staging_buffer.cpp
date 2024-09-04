
#include <cstring> // memcpy
#include <utility>

#include "staging_buffer.h"
#include "../device_cache.h"
#include "../utils/logger.h"
#include "../utils/sanity_check.h"

// TODO - Move this to a utility file
static bool IsMemoryOverlapping(void* _a, void* _b, size_t size)
{
	uint8_t* a = static_cast<uint8_t*>(_a);
	uint8_t* b = static_cast<uint8_t*>(_b);
	return ((a < b && a + size > b) || (b < a && b + size > a));
}

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

	void StagingBuffer::Create(VkDeviceSize size)
	{
		CreateBase(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}

	void StagingBuffer::Destroy()
	{
		VkDevice logicalDevice = GetLogicalDevice();

		if(buffer) vkDestroyBuffer(logicalDevice, buffer, nullptr);
		if(bufferMemory) vkFreeMemory(logicalDevice, bufferMemory, nullptr);

		buffer = VK_NULL_HANDLE;
		bufferMemory = VK_NULL_HANDLE;

		bufferState = BUFFER_STATE::DESTROYED;
	}

	void StagingBuffer::CopyIntoBuffer(void* sourceData, VkDeviceSize size)
	{
		VkDevice logicalDevice = GetLogicalDevice();

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

		if (IsMemoryOverlapping(bufferPtr, sourceData, size))
		{
			// Memory regions overlap, use memmove()
			LogInfo("Overlapping memory regions when copying data into staging buffer, using memmove()");
			memmove_s(bufferPtr, size, sourceData, size);
		}
		else
		{
			// Memory regions DON'T overlap, use memcpy()
			memcpy_s(bufferPtr, size, sourceData, size);
		}

		vkUnmapMemory(logicalDevice, bufferMemory);
	}

}