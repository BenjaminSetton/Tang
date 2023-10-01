
#include "../asset_types.h"
#include "index_buffer.h"
#include "../utils/sanity_check.h"

#include "vulkan/vulkan.h"

namespace TANG
{
	IndexBuffer::IndexBuffer() : stagingBuffer(VK_NULL_HANDLE), stagingBufferMemory(VK_NULL_HANDLE)
	{
		// Nothing to do here
	}

	IndexBuffer::~IndexBuffer()
	{
		// Nothing to do here
	}

	IndexBuffer::IndexBuffer(const IndexBuffer& other) : Buffer(other)
	{
		stagingBuffer = other.stagingBuffer;
		stagingBufferMemory = other.stagingBufferMemory;
	}

	IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept : Buffer(std::move(other))
	{
		stagingBuffer = other.stagingBuffer;
		stagingBufferMemory = other.stagingBufferMemory;

		other.stagingBuffer = VK_NULL_HANDLE;
		other.stagingBufferMemory = VK_NULL_HANDLE;
	}

	IndexBuffer& IndexBuffer::operator=(const IndexBuffer& other)
	{
		// Protect against self-assignment
		if (this == &other)
		{
			return *this;
		}

		Buffer::operator=(other);
		stagingBuffer = other.stagingBuffer;
		stagingBufferMemory = other.stagingBufferMemory;

		return *this;
	}

	void IndexBuffer::Create(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkDeviceSize size)
	{
		// Create the index buffer
		CreateBase(physicalDevice, logicalDevice, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		
		// Create the staging buffer
		CreateBase(physicalDevice, logicalDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);
	}

	void IndexBuffer::Destroy(VkDevice& logicalDevice)
	{
		// Destroy index buffer
		vkDestroyBuffer(logicalDevice, buffer, nullptr);
		vkFreeMemory(logicalDevice, bufferMemory, nullptr);

		buffer = VK_NULL_HANDLE;
		bufferMemory = VK_NULL_HANDLE;

		// Destroy staging buffer
		if (stagingBuffer) 
		{
			vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
			stagingBuffer = VK_NULL_HANDLE;
		}
		if (stagingBufferMemory)
		{
			vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
			stagingBufferMemory = VK_NULL_HANDLE;
		}

		bufferState = BUFFER_STATE::DESTROYED;
	}

	void IndexBuffer::DestroyIntermediateBuffers(VkDevice logicalDevice)
	{
		// Destroy staging buffer
		vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);

		stagingBuffer = VK_NULL_HANDLE;
		stagingBufferMemory = VK_NULL_HANDLE;
	}

	void IndexBuffer::CopyData(VkDevice& logicalDevice, VkCommandBuffer& commandBuffer, void* data, VkDeviceSize size)
	{
		// Map the memory
		void* bufferPtr;
		vkMapMemory(logicalDevice, stagingBufferMemory, 0, size, 0, &bufferPtr);
		memcpy(bufferPtr, data, size);
		vkUnmapMemory(logicalDevice, stagingBufferMemory);

		// Copy the data from the staging buffer into the vertex buffer
		CopyFromBuffer(commandBuffer, stagingBuffer, buffer, size);

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

