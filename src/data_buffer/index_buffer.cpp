
#include "../asset_types.h"
#include "index_buffer.h"
#include "staging_buffer.h"
#include "../utils/sanity_check.h"

#include "vulkan/vulkan.h"

namespace TANG
{
	IndexBuffer::IndexBuffer() : stagingBuffer(nullptr)
	{
	}

	IndexBuffer::~IndexBuffer()
	{
		if (stagingBuffer != nullptr)
		{
			LogWarning("Attempting to destroy index buffer while staging buffer is still in use!");
		}

		stagingBuffer = nullptr;
	}

	IndexBuffer::IndexBuffer(const IndexBuffer& other) : Buffer(other)
	{
	}

	IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept : Buffer(std::move(other)), stagingBuffer(std::move(other.stagingBuffer))
	{
	}

	IndexBuffer& IndexBuffer::operator=(const IndexBuffer& other)
	{
		// Protect against self-assignment
		if (this == &other)
		{
			return *this;
		}

		Buffer::operator=(other);

		return *this;
	}

	void IndexBuffer::Create(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkDeviceSize size)
	{
		// Create the index buffer
		CreateBase(physicalDevice, logicalDevice, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		
		// Create the staging buffer
		if (stagingBuffer == nullptr) stagingBuffer = new StagingBuffer();
		stagingBuffer->Create(physicalDevice, logicalDevice, size);
	}

	void IndexBuffer::Destroy(VkDevice& logicalDevice)
	{
		// Destroy index buffer
		vkDestroyBuffer(logicalDevice, buffer, nullptr);
		vkFreeMemory(logicalDevice, bufferMemory, nullptr);

		buffer = VK_NULL_HANDLE;
		bufferMemory = VK_NULL_HANDLE;

		DestroyIntermediateBuffers(logicalDevice);

		bufferState = BUFFER_STATE::DESTROYED;
	}

	void IndexBuffer::DestroyIntermediateBuffers(VkDevice logicalDevice)
	{
		if (stagingBuffer != nullptr)
		{
			stagingBuffer->Destroy(logicalDevice);
			delete stagingBuffer;
			stagingBuffer = nullptr;
		}
	}

	void IndexBuffer::CopyData(VkDevice& logicalDevice, VkCommandBuffer& commandBuffer, void* data, VkDeviceSize size)
	{
		if (stagingBuffer == nullptr)
		{
			LogWarning("Attempting to copy data into index buffer, but staging buffer has not been created!");
			return;
		}

		// Map the memory
		void* bufferPtr;
		vkMapMemory(logicalDevice, stagingBuffer->GetBufferMemory(), 0, size, 0, &bufferPtr);
		memcpy(bufferPtr, data, size);
		vkUnmapMemory(logicalDevice, stagingBuffer->GetBufferMemory());

		// Copy the data from the staging buffer into the vertex buffer
		CopyFromBuffer(commandBuffer, stagingBuffer->GetBuffer(), buffer, size);

		bufferState = BUFFER_STATE::MAPPED;
	}

	VkIndexType IndexBuffer::GetIndexType() const
	{
		switch (sizeof(IndexType))
		{
		case 2: return VK_INDEX_TYPE_UINT16;
		case 4: return VK_INDEX_TYPE_UINT32;
		}

		TNG_ASSERT_MSG(false, "Unsupported index type!");
		return VK_INDEX_TYPE_MAX_ENUM;
	}
}

