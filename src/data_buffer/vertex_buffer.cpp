
#include <string.h> // memcpy
#include <utility> // numeric_limits

#include "../device_cache.h"
#include "../utils/logger.h"
#include "staging_buffer.h"
#include "vertex_buffer.h"

namespace TANG
{
	VertexBuffer::VertexBuffer()
	{
	}

	VertexBuffer::~VertexBuffer()
	{
		if (!stagingBuffer.IsInvalid())
		{
			LogWarning("Attempting to destroy vertex buffer while staging buffer is still in use!");
		}
	}

	VertexBuffer::VertexBuffer(const VertexBuffer& other) : Buffer(other)
	{
	}

	VertexBuffer::VertexBuffer(VertexBuffer&& other) : Buffer(std::move(other)), stagingBuffer(std::move(other.stagingBuffer))
	{
	}

	VertexBuffer& VertexBuffer::operator=(const VertexBuffer& other)
	{
		// Protect against self-assignment
		if (this == &other)
		{
			return *this;
		}

		Buffer::operator=(other);

		return *this;
	}

	void VertexBuffer::Create(VkDeviceSize size)
	{
		// Create the vertex buffer
		CreateBase(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		
		// Create the staging buffer
		stagingBuffer.Create(size);
	}

	void VertexBuffer::Destroy()
	{
		VkDevice logicalDevice = GetLogicalDevice();

		// Destroy vertex buffer
		vkDestroyBuffer(logicalDevice, buffer, nullptr);
		vkFreeMemory(logicalDevice, bufferMemory, nullptr);

		buffer = VK_NULL_HANDLE;
		bufferMemory = VK_NULL_HANDLE;

		DestroyIntermediateBuffers();

		bufferState = BUFFER_STATE::DESTROYED;
	}

	void VertexBuffer::DestroyIntermediateBuffers()
	{
		stagingBuffer.Destroy();
	}

	void VertexBuffer::CopyIntoBuffer(VkCommandBuffer commandBuffer, void* sourceData, VkDeviceSize size)
	{
		VkDevice logicalDevice = GetLogicalDevice();

		if (stagingBuffer.IsInvalid())
		{
			LogWarning("Attempting to copy data into vertex buffer, but staging buffer has not been created!");
			return;
		}

		// Map the memory
		void* bufferPtr;
		VkResult res = vkMapMemory(logicalDevice, stagingBuffer.GetBufferMemory(), 0, size, 0, &bufferPtr);
		if (res != VK_SUCCESS)
		{
			LogError("Failed to map memory for staging buffer!");
		}
		memcpy(bufferPtr, sourceData, size);
		vkUnmapMemory(logicalDevice, stagingBuffer.GetBufferMemory());

		// Copy the data from the staging buffer into the vertex buffer
		CopyFromBuffer(commandBuffer, stagingBuffer.GetBuffer(), buffer, size);

		bufferState = BUFFER_STATE::MAPPED;
	}
}


