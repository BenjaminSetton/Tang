
#include <utility> // std::move

#include "../device_cache.h"
#include "shader_storage_buffer.h"

namespace TANG
{

	ShaderStorageBuffer::ShaderStorageBuffer(VkBufferUsageFlags _extraUsage) : Buffer(), extraUsage(_extraUsage)
	{ }

	ShaderStorageBuffer::~ShaderStorageBuffer()
	{ }

	ShaderStorageBuffer::ShaderStorageBuffer(const ShaderStorageBuffer& other) : Buffer(other), extraUsage(other.extraUsage)
	{ }

	ShaderStorageBuffer::ShaderStorageBuffer(ShaderStorageBuffer&& other) noexcept : Buffer(std::move(other)), extraUsage(std::move(other.extraUsage))
	{
		extraUsage = 0;
	}

	ShaderStorageBuffer& ShaderStorageBuffer::operator=(const ShaderStorageBuffer& other)
	{
		if (this == &other)
		{
			return *this;
		}

		Buffer::operator=(other);
		extraUsage = other.extraUsage;

		return *this;
	}

	void ShaderStorageBuffer::Create(VkDeviceSize size)
	{
		// We're creating a device local buffer (meaning local to the GPU). Therefore, we need to ensure it's usage is set to TRANSFER_DST
		// because we need to transfer data from the host (CPU) to this device local buffer
		CreateBase(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | extraUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	void ShaderStorageBuffer::Destroy()
	{
		if (bufferState != BUFFER_STATE::CREATED)
		{
			// Can't destroy a buffer that's not been created
			return;
		}

		VkDevice logicalDevice = GetLogicalDevice();

		vkUnmapMemory(logicalDevice, bufferMemory);
		vkDestroyBuffer(logicalDevice, buffer, nullptr);
		vkFreeMemory(logicalDevice, bufferMemory, nullptr);

		buffer = VK_NULL_HANDLE;
		bufferMemory = VK_NULL_HANDLE;
		bufferSize = 0;
		bufferState = BUFFER_STATE::DESTROYED;
	}
}