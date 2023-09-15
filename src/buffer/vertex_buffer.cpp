
#include "vertex_buffer.h"

namespace TANG
{
	VertexBuffer::VertexBuffer() { }
	VertexBuffer::~VertexBuffer() { }
	VertexBuffer::VertexBuffer(const VertexBuffer& other) { }

	void VertexBuffer::Create(VkDevice& logicalDevice, VkDeviceSize size)
	{
		CreateBase(logicalDevice, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	void VertexBuffer::MapData(VkDevice& logicalDevice, VkCommandBuffer& commandBuffer, void* data, VkDeviceSize bufferSize)
	{
		// Create the staging buffer
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBase(logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		// Map the memory
		void* bufferPtr;
		vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &bufferPtr);
		memcpy(bufferPtr, data, bufferSize);
		vkUnmapMemory(logicalDevice, stagingBufferMemory);

		// Create the vertex buffer
		CreateBase(logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// Copy the data from the staging buffer into the vertex buffer
		CopyFromBuffer(commandBuffer, stagingBuffer, buffer, bufferSize);

		// Destroy the staging buffer since we no longer need it
		vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
	}
}


