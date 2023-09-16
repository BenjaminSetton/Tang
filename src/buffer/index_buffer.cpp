
#include "index_buffer.h"

namespace TANG
{
	IndexBuffer::IndexBuffer() { }
	IndexBuffer::~IndexBuffer() { }
	IndexBuffer::IndexBuffer(const IndexBuffer& other) { }

	void IndexBuffer::Create(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkDeviceSize size)
	{
		CreateBase(physicalDevice, logicalDevice, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	void IndexBuffer::MapData(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkCommandBuffer& commandBuffer, void* data, VkDeviceSize bufferSize)
	{
		// Create the staging buffer
		VkBuffer stagingBuffer{};
		VkDeviceMemory stagingBufferMemory{};
		CreateBase(physicalDevice, logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

		// Map the memory
		void* bufferPtr;
		vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &bufferPtr);
		memcpy(bufferPtr, data, bufferSize);
		vkUnmapMemory(logicalDevice, stagingBufferMemory);

		// Copy the data from the staging buffer into the vertex buffer
		CopyFromBuffer(commandBuffer, stagingBuffer, buffer, bufferSize);

		// Destroy the staging buffer since we no longer need it
		vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
	}
}

