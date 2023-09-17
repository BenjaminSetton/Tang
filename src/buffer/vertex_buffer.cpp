
#include "vertex_buffer.h"
#include "../utils/logger.h"

namespace TANG
{
	VertexBuffer::VertexBuffer() { }
	VertexBuffer::~VertexBuffer() { }
	VertexBuffer::VertexBuffer(const VertexBuffer& other) { }

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

		// Destroy staging buffer
		if(stagingBuffer) vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		if(stagingBufferMemory) vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
	}

	void VertexBuffer::DestroyIntermediateBuffers(VkDevice logicalDevice)
	{
		// Destroy staging buffer
		vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);

		stagingBuffer = VK_NULL_HANDLE;
		stagingBufferMemory = VK_NULL_HANDLE;
	}

	void VertexBuffer::MapData(VkDevice& logicalDevice, VkCommandBuffer& commandBuffer, void* data, VkDeviceSize bufferSize)
	{
		// Map the memory
		void* bufferPtr;
		VkResult res = vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &bufferPtr);
		if (res != VK_SUCCESS)
		{
			LogError("Failed to map memory for staging buffer!");
		}
		memcpy(bufferPtr, data, bufferSize);
		vkUnmapMemory(logicalDevice, stagingBufferMemory);

		// Copy the data from the staging buffer into the vertex buffer
		CopyFromBuffer(commandBuffer, stagingBuffer, buffer, bufferSize);
	}
}


