#ifndef VERTEX_BUFFER_H
#define VERTEX_BUFFER_H

#include "staging_buffer.h"

namespace TANG
{
	class VertexBuffer : public Buffer
	{
	public:

		VertexBuffer();
		~VertexBuffer();
		VertexBuffer(const VertexBuffer& other);
		VertexBuffer(VertexBuffer&& other);
		VertexBuffer& operator=(const VertexBuffer& other);

		void Create(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkDeviceSize size) override;

		void Destroy(VkDevice logicalDevice) override;

		void DestroyIntermediateBuffers(VkDevice logicalDevice);

		void CopyIntoBuffer(VkDevice logicalDevice, VkCommandBuffer commandBuffer, void* sourceData, VkDeviceSize size);

	private:

		// Store the staging buffer so that we can delete it properly after ending and submitting the command buffer
		StagingBuffer stagingBuffer;
	};
}

#endif