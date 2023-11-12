#ifndef INDEX_BUFFER_H
#define INDEX_BUFFER_H

#include "buffer.h"

enum VkIndexType;

namespace TANG
{
	class StagingBuffer;

	class IndexBuffer : public Buffer
	{
	public:

		IndexBuffer();
		~IndexBuffer();
		IndexBuffer(const IndexBuffer& other);
		IndexBuffer(IndexBuffer&& other) noexcept;
		IndexBuffer& operator=(const IndexBuffer& other);

		void Create(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkDeviceSize size) override;

		void Destroy(VkDevice& logicalDevice) override;

		void DestroyIntermediateBuffers(VkDevice logicalDevice);

		void CopyData(VkDevice& logicalDevice, VkCommandBuffer& commandBuffer, void* data, VkDeviceSize bufferSize);

		VkIndexType GetIndexType() const;

	private:

		// Store the staging buffer so that we can delete it properly after ending and submitting the command buffer
		StagingBuffer* stagingBuffer;
	};
}

#endif