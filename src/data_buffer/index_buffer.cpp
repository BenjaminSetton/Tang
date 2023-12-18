
#include "index_buffer.h"
#include "staging_buffer.h"
#include "../asset_types.h"
#include "../device_cache.h"
#include "../utils/sanity_check.h"

#include "vulkan/vulkan.h"

namespace TANG
{
	IndexBuffer::IndexBuffer()
	{
	}

	IndexBuffer::~IndexBuffer()
	{
		if (!stagingBuffer.IsInvalid())
		{
			LogWarning("Attempting to destroy index buffer while staging buffer is still in use!");
		}
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

	void IndexBuffer::Create(VkDeviceSize size)
	{
		// Create the index buffer
		CreateBase(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		
		stagingBuffer.Create(size);
	}

	void IndexBuffer::Destroy()
	{
		VkDevice logicalDevice = GetLogicalDevice();

		vkDestroyBuffer(logicalDevice, buffer, nullptr);
		vkFreeMemory(logicalDevice, bufferMemory, nullptr);

		buffer = VK_NULL_HANDLE;
		bufferMemory = VK_NULL_HANDLE;

		DestroyIntermediateBuffers();

		bufferState = BUFFER_STATE::DESTROYED;
	}

	void IndexBuffer::DestroyIntermediateBuffers()
	{
		stagingBuffer.Destroy();
	}

	void IndexBuffer::CopyIntoBuffer(VkCommandBuffer commandBuffer, void* sourceData, VkDeviceSize size)
	{
		VkDevice logicalDevice = GetLogicalDevice();

		if (stagingBuffer.IsInvalid())
		{
			LogWarning("Attempting to copy data into index buffer, but staging buffer has not been created!");
			return;
		}

		// Map the memory
		void* bufferPtr;
		vkMapMemory(logicalDevice, stagingBuffer.GetBufferMemory(), 0, size, 0, &bufferPtr);
		memcpy(bufferPtr, sourceData, size);
		vkUnmapMemory(logicalDevice, stagingBuffer.GetBufferMemory());

		CopyFromBuffer(commandBuffer, stagingBuffer.GetBuffer(), buffer, size);

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

