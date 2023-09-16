
#include "buffer.h"
#include "../utils/logger.h"

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
	VkBuffer Buffer::GetBuffer()
	{
		return buffer;
	}

	// Returns the member variable "bufferMemory"
	VkDeviceMemory Buffer::GetBufferMemory()
	{
		return bufferMemory;
	}

	void Buffer::CopyFromBuffer(VkCommandBuffer& commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
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