#ifndef UNIFORM_BUFFER_H
#define UNIFORM_BUFFER_H

#include "buffer.h"

namespace TANG
{
	class UniformBuffer : public Buffer 
	{
	public:

		UniformBuffer();
		~UniformBuffer();
		UniformBuffer(const UniformBuffer& other);
		UniformBuffer(UniformBuffer&& other) noexcept;
		UniformBuffer& operator=(const UniformBuffer& other);

		void Create(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkDeviceSize size) override;

		void Destroy(VkDevice logicalDevice) override;

		void MapMemory(VkDevice logicalDevice, VkDeviceSize bufferSize);
		void UnMapMemory(VkDevice logicalDevice);

		void UpdateData(void* data, uint32_t numBytes);

		void* GetMappedData();

	private:

		void* mappedData;
	};
}

#endif