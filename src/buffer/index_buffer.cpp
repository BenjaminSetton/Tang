
#include "index_buffer.h"

namespace TANG
{
	IndexBuffer::IndexBuffer() { }
	IndexBuffer::~IndexBuffer() { }
	IndexBuffer::IndexBuffer(const IndexBuffer& other) { }

	void IndexBuffer::Create(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkDeviceSize size)
	{
		// Create the index buffer
		CreateBase(physicalDevice, logicalDevice, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		
		// Create the staging buffer
		CreateBase(physicalDevice, logicalDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);
	}

	void IndexBuffer::Destroy(VkDevice& logicalDevice)
	{
		// Destroy index buffer
		vkDestroyBuffer(logicalDevice, buffer, nullptr);
		vkFreeMemory(logicalDevice, bufferMemory, nullptr);

		// Destroy staging buffer
		if(stagingBuffer) vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		if(stagingBufferMemory) vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
	}

	void IndexBuffer::DestroyIntermediateBuffers(VkDevice logicalDevice)
	{
		// Destroy staging buffer
		vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);

		stagingBuffer = VK_NULL_HANDLE;
		stagingBufferMemory = VK_NULL_HANDLE;
	}

	void IndexBuffer::MapData(VkDevice& logicalDevice, VkCommandBuffer& commandBuffer, void* data, VkDeviceSize bufferSize)
	{
		// Map the memory
		void* bufferPtr;
		vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &bufferPtr);
		memcpy(bufferPtr, data, bufferSize);
		vkUnmapMemory(logicalDevice, stagingBufferMemory);

		// Copy the data from the staging buffer into the vertex buffer
		CopyFromBuffer(commandBuffer, stagingBuffer, buffer, bufferSize);
	}
}

