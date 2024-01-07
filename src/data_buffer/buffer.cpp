
#include <limits>

#include "buffer.h"
#include "../utils/logger.h"
#include "../device_cache.h"

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
		if (other.bufferState == BUFFER_STATE::DESTROYED)
		{
			LogWarning("Why are we attempting to copy from a destroyed buffer? Bailing...");
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
		if (other.bufferState == BUFFER_STATE::DESTROYED)
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
		if (this == &other)
		{
			return *this;
		}

		if (other.bufferState == BUFFER_STATE::DESTROYED)
		{
			LogWarning("Why are we attempting to copy from a destroyed buffer? Bailing...");
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

	const VkBuffer Buffer::GetBuffer() const 
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

	bool Buffer::IsInvalid() const
	{
		return bufferState == BUFFER_STATE::DEFAULT || bufferState == BUFFER_STATE::DESTROYED;
	}

	void Buffer::CopyFromBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	}

	void Buffer::CreateBase(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
	{
		VkDevice logicalDevice = GetLogicalDevice();

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		{
			LogError("Failed to create buffer!");
			return;
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(logicalDevice, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		{
			LogError("Failed to allocate memory for the buffer!");
			return;
		}

		vkBindBufferMemory(logicalDevice, buffer, bufferMemory, 0);

		bufferSize = size;

		bufferState = BUFFER_STATE::CREATED;
	}

	uint32_t Buffer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memFlags)
	{
		VkPhysicalDeviceMemoryProperties memProperties = DeviceCache::Get().GetPhysicalDeviceMemoryProperties();

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