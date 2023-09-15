#ifndef VERTEX_BUFFER_H
#define VERTEX_BUFFER_H

#include "buffer.h"

namespace TANG
{
	class VertexBuffer : Buffer
	{
	public:

		VertexBuffer();
		~VertexBuffer();
		VertexBuffer(const VertexBuffer& other);

		void Create(VkDevice& logicalDevice, VkDeviceSize size);

		void MapData(VkDevice& logicalDevice, VkCommandBuffer& commandBuffer, void* data, VkDeviceSize bufferSize);

	private:
	};
}

#endif