
#include <cstring> //memcpy
#include <utility> // std::move

#include "uniform_buffer.h"
#include "../utils/logger.h"

namespace TANG
{
	UniformBuffer::UniformBuffer() : mappedData(nullptr)
	{

	}

	UniformBuffer::~UniformBuffer()
	{
		// Nothing to do here
	}

	UniformBuffer::UniformBuffer(const UniformBuffer& other) : Buffer(other)
	{
		mappedData = other.mappedData;
	}

	UniformBuffer::UniformBuffer(UniformBuffer&& other) noexcept : Buffer(std::move(other))
	{
		mappedData = other.mappedData;

		other.mappedData = nullptr;
	}

	UniformBuffer& UniformBuffer::operator=(const UniformBuffer& other)
	{
		// Protect against self-assignment
		if (this == &other)
		{
			return *this;
		}

		mappedData = other.mappedData;
		return *this;
	}

	void UniformBuffer::Create(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkDeviceSize size)
	{
		CreateBase(physicalDevice, logicalDevice, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}

	void UniformBuffer::Destroy(VkDevice& logicalDevice)
	{
		if (mappedData != nullptr)
		{
			vkUnmapMemory(logicalDevice, bufferMemory);
		}

		mappedData = nullptr;

		vkDestroyBuffer(logicalDevice, buffer, nullptr);
		vkFreeMemory(logicalDevice, bufferMemory, nullptr);

		buffer = VK_NULL_HANDLE;
		bufferMemory = VK_NULL_HANDLE;

		bufferState = BUFFER_STATE::DESTROYED;
	}

	void UniformBuffer::MapMemory(VkDevice& logicalDevice, VkDeviceSize bufferSize)
	{
		vkMapMemory(logicalDevice, bufferMemory, 0, bufferSize, 0, &mappedData);
	}

	void UniformBuffer::UpdateData(void* data, uint32_t numBytes)
	{
		if (bufferState != BUFFER_STATE::MAPPED)
		{
			LogError("Attempting to update data on uniform buffer when it's not mapped. Data will not be updated");
		}

		memcpy(mappedData, data, numBytes);
	}

	void* UniformBuffer::GetMappedData()
	{
		return mappedData;
	}

}


