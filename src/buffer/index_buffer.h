#ifndef INDEX_BUFFER_H
#define INDEX_BUFFER_H

#include "buffer.h"

namespace TANG
{
	class IndexBuffer : public Buffer
	{
	public:

		IndexBuffer();
		~IndexBuffer();
		IndexBuffer(const IndexBuffer& other);

		void Create(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkDeviceSize size) override;

		void MapData(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkCommandBuffer& commandBuffer, void* data, VkDeviceSize bufferSize) override;

	private:
	};
}

#endif