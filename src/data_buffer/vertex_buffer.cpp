
#include <string.h> // memcpy
#include <utility> // numeric_limits

#include "vertex_buffer.h"
#include "../utils/logger.h"

namespace TANG
{
	VertexBuffer::VertexBuffer() : stagingBuffer(VK_NULL_HANDLE), stagingBufferMemory(VK_NULL_HANDLE)
	{
		// Nothing to do here
	}

	VertexBuffer::~VertexBuffer()
	{
		// Nothing to do here
	}

	VertexBuffer::VertexBuffer(const VertexBuffer& other) : Buffer(other)
	{
		stagingBuffer = VK_NULL_HANDLE;
		stagingBufferMemory = VK_NULL_HANDLE;
	}

	VertexBuffer::VertexBuffer(VertexBuffer&& other) : Buffer(std::move(other))
	{
		stagingBuffer = other.stagingBuffer;
		stagingBufferMemory = other.stagingBufferMemory;

		other.stagingBuffer = VK_NULL_HANDLE;
		other.stagingBufferMemory = VK_NULL_HANDLE;
	}

	VertexBuffer& VertexBuffer::operator=(const VertexBuffer& other)
	{
		// Protect against self-assignment
		if (this == &other)
		{
			return *this;
		}

		Buffer::operator=(other);
		stagingBuffer = VK_NULL_HANDLE;
		stagingBufferMemory = VK_NULL_HANDLE;

		return *this;
	}

	void VertexBuffer::Create(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkDeviceSize size)
	{
		// Create the vertex buffer
		CreateBase(physicalDevice, logicalDevice, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		
		// Create the staging buffer
		CreateBase(physicalDevice, logicalDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);
	}

	void VertexBuffer::Destroy(VkDevice& logicalDevice)
	{
		// Destroy vertex buffer
		vkDestroyBuffer(logicalDevice, buffer, nullptr);
		vkFreeMemory(logicalDevice, bufferMemory, nullptr);

		buffer = VK_NULL_HANDLE;
		bufferMemory = VK_NULL_HANDLE;

		// Destroy staging buffer
		if(stagingBuffer) vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		if(stagingBufferMemory) vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);

		stagingBuffer = VK_NULL_HANDLE;
		stagingBufferMemory = VK_NULL_HANDLE;

		bufferState = BUFFER_STATE::DESTROYED;
	}

	void VertexBuffer::DestroyIntermediateBuffers(VkDevice logicalDevice)
	{
		// Destroy staging buffer
		vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);

		stagingBuffer = VK_NULL_HANDLE;
		stagingBufferMemory = VK_NULL_HANDLE;
	}

	void VertexBuffer::CopyData(VkDevice& logicalDevice, VkCommandBuffer& commandBuffer, void* data, VkDeviceSize size)
	{
		// Map the memory
		void* bufferPtr;
		VkResult res = vkMapMemory(logicalDevice, stagingBufferMemory, 0, size, 0, &bufferPtr);
		if (res != VK_SUCCESS)
		{
			LogError("Failed to map memory for staging buffer!");
		}
		memcpy(bufferPtr, data, size);
		vkUnmapMemory(logicalDevice, stagingBufferMemory);

		// Copy the data from the staging buffer into the vertex buffer
		CopyFromBuffer(commandBuffer, stagingBuffer, buffer, size);

		bufferState = BUFFER_STATE::MAPPED;
	}
}


