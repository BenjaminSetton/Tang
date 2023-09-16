
#include "vertex_buffer.h"

namespace TANG
{
	VertexBuffer::VertexBuffer() { }
	VertexBuffer::~VertexBuffer() { }
	VertexBuffer::VertexBuffer(const VertexBuffer& other) { }

	void VertexBuffer::Create(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkDeviceSize size)
	{
		CreateBase(physicalDevice, logicalDevice, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	void VertexBuffer::MapData(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkCommandBuffer& commandBuffer, void* data, VkDeviceSize bufferSize)
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

		// Insert pipeline barrier to wait for the copy operation to finish
		VkBufferMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.buffer = buffer;
		barrier.offset = 0;
		barrier.size = bufferSize;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, // Transfer stage (assuming you're using transfer queue)
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, // Vertex input stage
			0,
			0, nullptr,
			1, &barrier,
			0, nullptr
		);

		// Destroy the staging buffer since we no longer need it
		vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
	}
}


