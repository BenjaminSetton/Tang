
#include <cstring> //memcpy
#include <utility> // std::move

#include "../device_cache.h"
#include "../utils/logger.h"
#include "uniform_buffer.h"

static const VkDeviceSize PREFERRED_UNIFORM_BUFFER_MIN_SIZE = 128;

namespace TANG
{
	UniformBuffer::UniformBuffer() : mappedData(nullptr), bufferSize(0)
	{ }

	UniformBuffer::~UniformBuffer()
	{ }

	UniformBuffer::UniformBuffer(const UniformBuffer& other) : Buffer(other), mappedData(other.mappedData), bufferSize(other.bufferSize)
	{ }

	UniformBuffer::UniformBuffer(UniformBuffer&& other) noexcept : Buffer(std::move(other)), mappedData(std::move(other.mappedData)), bufferSize(std::move(other.bufferSize))
	{ }

	UniformBuffer& UniformBuffer::operator=(const UniformBuffer& other)
	{
		// Protect against self-assignment
		if (this == &other)
		{
			return *this;
		}

		mappedData = other.mappedData;
		bufferSize = other.bufferSize;
		return *this;
	}

	void UniformBuffer::Create(VkDeviceSize size)
	{
		if (size < PREFERRED_UNIFORM_BUFFER_MIN_SIZE)
		{
			LogWarning("Creating uniform buffer of size %u, which is less than the preferred minimum of %u. Prefer push constants instead!", size, PREFERRED_UNIFORM_BUFFER_MIN_SIZE);
		}

		CreateBase(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		bufferSize = size;
	}

	void UniformBuffer::Destroy()
	{
		VkDevice logicalDevice = GetLogicalDevice();

		if (mappedData != nullptr)
		{
			vkUnmapMemory(logicalDevice, bufferMemory);
		}

		bufferSize = 0;
		mappedData = nullptr;

		vkDestroyBuffer(logicalDevice, buffer, nullptr);
		vkFreeMemory(logicalDevice, bufferMemory, nullptr);

		buffer = VK_NULL_HANDLE;
		bufferMemory = VK_NULL_HANDLE;

		bufferState = BUFFER_STATE::DESTROYED;
	}

	void UniformBuffer::MapMemory(VkDeviceSize size)
	{
		// If a size of 0 is specified, then we assume the caller wants to map the entirety of the buffer size (as opposed to a portion)
		if (size == 0)
		{
			size = bufferSize;
		}

		vkMapMemory(GetLogicalDevice(), bufferMemory, 0, size, 0, &mappedData);
		bufferState = BUFFER_STATE::MAPPED;
	}

	void UniformBuffer::UnMapMemory()
	{
		if (bufferState != BUFFER_STATE::MAPPED)
		{
			LogError("Failed to unmap uniform buffer memory. Memory was not mapped to begin with!");
			return;
		}

		vkUnmapMemory(GetLogicalDevice(), bufferMemory);
		bufferState = BUFFER_STATE::CREATED;
		mappedData = nullptr;
	}

	void UniformBuffer::UpdateData(void* data, uint32_t numBytes)
	{
		if (bufferState != BUFFER_STATE::MAPPED)
		{
			LogError("Attempting to update data on uniform buffer when it's not mapped. Data will not be updated");
			return;
		}

		memcpy(mappedData, data, numBytes);
	}

	void* UniformBuffer::GetMappedData()
	{
		return mappedData;
	}

}


