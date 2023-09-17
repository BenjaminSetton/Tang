#ifndef VERTEX_BUFFER_H
#define VERTEX_BUFFER_H

#include "buffer.h"

namespace TANG
{
	class VertexBuffer : public Buffer
	{
	public:

		VertexBuffer();
		~VertexBuffer();
		VertexBuffer(const VertexBuffer& other);

		void Create(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkDeviceSize size) override;

		void Destroy(VkDevice& logicalDevice) override;

		void DestroyIntermediateBuffers(VkDevice logicalDevice);

		void MapData(VkDevice& logicalDevice, VkCommandBuffer& commandBuffer, void* data, VkDeviceSize bufferSize) override;

	private:

		// Store the staging buffer so that we can delete it properly after ending and submitting the command buffer
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
	};
}

#endif