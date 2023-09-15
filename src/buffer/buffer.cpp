
#include "buffer.h"
#include "logger.h"

namespace TANG
{
	Buffer::Buffer() { }
	Buffer::~Buffer() { }
	Buffer::Buffer(const Buffer& other) { }

	void Buffer::Destroy(VkDevice& logicalDevice)
	{
		// NOTE - This assumes we did not use a custom allocator
		vkDestroyBuffer(logicalDevice, buffer, nullptr);
		vkFreeMemory(logicalDevice, bufferMemory, nullptr);
	}

	// Returns the member variable "buffer"
	VkBuffer Buffer::GetBuffer() const
	{
		return buffer;
	}

	// Returns the member variable "bufferMemory"
	VkDeviceMemory Buffer::GetBufferMemory() const
	{
		return bufferMemory;
	}

	void Buffer::CopyFromBuffer(VkCommandBuffer& commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	}

	void Buffer::CreateBase(VkDevice& logicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, uint32_t memoryTypeIndex, VkBuffer _buffer = nullptr, VkDeviceMemory _bufferMemory = nullptr)
	{
		// Determine the output VkBuffer and VkDeviceMemory
		VkBuffer& endBuffer = _buffer == nullptr ? buffer : _buffer;
		VkDeviceMemory& endBufferMemory = _bufferMemory == nullptr ? _bufferMemory : _bufferMemory;

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &endBuffer) != VK_SUCCESS)
		{
			LogError("Failed to create buffer!");
			return;
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(logicalDevice, endBuffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = memoryTypeIndex;

		if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &endBufferMemory) != VK_SUCCESS)
		{
			LogError("Failed to allocate memory for the buffer!");
			return;
		}

		vkBindBufferMemory(logicalDevice, endBuffer, endBufferMemory, 0);
	}
}