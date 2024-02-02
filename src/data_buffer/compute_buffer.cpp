
#include <utility> // std::move

#include "../device_cache.h"
#include "compute_buffer.h"

namespace TANG
{

	ComputeBuffer::ComputeBuffer() : Buffer()
	{ }

	ComputeBuffer::~ComputeBuffer()
	{ }

	ComputeBuffer::ComputeBuffer(const ComputeBuffer& other) : Buffer(other)
	{ }

	ComputeBuffer::ComputeBuffer(ComputeBuffer&& other) noexcept : Buffer(std::move(other))
	{ }

	ComputeBuffer& ComputeBuffer::operator=(const ComputeBuffer& other)
	{
		Buffer::operator=(other);
		return *this;
	}

	void ComputeBuffer::Create(VkDeviceSize size)
	{
		// We're creating a buffer of a specific size which is used as a storage buffer on the device (GPU)
		CreateBase(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	void ComputeBuffer::Destroy()
	{
		if (bufferState != BUFFER_STATE::CREATED)
		{
			// Can't destroy a buffer that's not been created
			return;
		}

		VkDevice logicalDevice = GetLogicalDevice();

		vkUnmapMemory(logicalDevice, bufferMemory);
		vkDestroyBuffer(logicalDevice, buffer, nullptr);
		vkFreeMemory(logicalDevice, bufferMemory, nullptr);

		buffer = VK_NULL_HANDLE;
		bufferMemory = VK_NULL_HANDLE;
		bufferSize = 0;
		bufferState = BUFFER_STATE::DESTROYED;
	}
}