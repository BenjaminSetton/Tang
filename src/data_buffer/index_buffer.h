#ifndef INDEX_BUFFER_H
#define INDEX_BUFFER_H

#include "staging_buffer.h"

enum VkIndexType;

namespace TANG
{
	class IndexBuffer : public Buffer
	{
	public:

		IndexBuffer();
		~IndexBuffer();
		IndexBuffer(const IndexBuffer& other);
		IndexBuffer(IndexBuffer&& other) noexcept;
		IndexBuffer& operator=(const IndexBuffer& other);

		void Create(VkDeviceSize size) override;

		void Destroy() override;

		void DestroyIntermediateBuffers();

		void CopyIntoBuffer(VkCommandBuffer commandBuffer, void* sourceData, VkDeviceSize bufferSize);

		VkIndexType GetIndexType() const;

	private:

		// Store the staging buffer so that we can delete it properly after ending and submitting the command buffer
		StagingBuffer stagingBuffer;
	};
}

#endif