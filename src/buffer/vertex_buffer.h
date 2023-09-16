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

		void MapData(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkCommandBuffer& commandBuffer, void* data, VkDeviceSize bufferSize) override;

	private:
	};
}

#endif