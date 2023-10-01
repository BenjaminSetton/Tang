
#include <limits>

#include "buffer.h"
#include "../utils/logger.h"

namespace TANG
{
	Buffer::Buffer() : buffer(VK_NULL_HANDLE), bufferMemory(VK_NULL_HANDLE), bufferSize(0), bufferState(BUFFER_STATE::DEFAULT)
	{
	}

	Buffer::~Buffer()
	{
		if (bufferState != BUFFER_STATE::MAPPED)
		{
			buffer = VK_NULL_HANDLE;
			bufferMemory = VK_NULL_HANDLE;
			bufferSize = 0;
		}
		else
		{
			// TODO - We shouldn't warn if the object was copied to another place
			LogWarning("Buffer destructor was called but memory has not been cleaned up!");
		}
	}

	Buffer::Buffer(const Buffer& other)
	{
		if (bufferState == BUFFER_STATE::DESTROYED)
		{
			LogWarning("Why are we attempting to copy a destroyed buffer? Bailing...");
			return;
		}

		// Note that after this copy, there are two (or more) handles to the same buffer and it's associated memory
		// Careful when trying to access the internal buffer, as other handles could delete this memory!
		buffer = other.buffer;
		bufferMemory = other.bufferMemory;
		bufferSize = other.bufferSize;
		bufferState = other.bufferState;
	}

	Buffer::Buffer(Buffer&& other) noexcept
	{
		if (bufferState == BUFFER_STATE::DESTROYED)
		{
			LogWarning("Why are we attempting to move a destroyed buffer? Bailing...");
			return;
		}

		buffer = other.buffer;
		bufferMemory = other.bufferMemory;
		bufferSize = other.bufferSize;
		bufferState = other.bufferState;

		// Remove references in other buffer
		other.buffer = VK_NULL_HANDLE;
		other.bufferMemory = VK_NULL_HANDLE;
		other.bufferSize = 0;
	}

	Buffer& Buffer::operator=(const Buffer& other)
	{
		if (bufferState == BUFFER_STATE::DESTROYED)
		{
			LogWarning("Why are we attempting to copy a destroyed buffer? Bailing...");
			return *this;
		}

		// Protect against self-assignment
		if (this == &other)
		{
			return *this;
		}

		// Note that after this copy, there are two (or more) handles to the same buffer and it's associated memory
		// Careful when trying to access the internal buffer, as other handles could delete this memory!
		buffer = other.buffer;
		bufferMemory = other.bufferMemory;
		bufferSize = other.bufferSize;
		bufferState = other.bufferState;

		return *this;
	}

	// Returns the member variable "buffer"
	VkBuffer Buffer::GetBuffer()
	{
		return buffer;
	}

	// Returns the member variable "bufferMemory"
	VkDeviceMemory Buffer::GetBufferMemory()
	{
		return bufferMemory;
	}

	VkDeviceSize Buffer::GetBufferSize() const
	{
		return bufferSize;
	}

	void Buffer::CopyFromBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	}

	void Buffer::CreateBase(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* outBuffer, VkDeviceMemory* outBufferMemory)
	{
		// Determine the output VkBuffer and VkDeviceMemory
		VkBuffer* endBuffer = outBuffer == nullptr ? &buffer : outBuffer;
		VkDeviceMemory* endBufferMemory = outBufferMemory == nullptr ? &bufferMemory : outBufferMemory;

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, endBuffer) != VK_SUCCESS)
		{
			LogError("Failed to create buffer!");
			return;
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(logicalDevice, *endBuffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);

		if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, endBufferMemory) != VK_SUCCESS)
		{
			LogError("Failed to allocate memory for the buffer!");
			return;
		}

		vkBindBufferMemory(logicalDevice, *endBuffer, *endBufferMemory, 0);

		bufferSize = size;

		bufferState = BUFFER_STATE::CREATED;
	}

	uint32_t Buffer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memFlags, VkPhysicalDevice& physicalDevice)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & memFlags) == memFlags)
			{
				return i;
			}
		}

		LogError("Failed to find suitable memory type!");
		return std::numeric_limits<uint32_t>::max();
	}
}