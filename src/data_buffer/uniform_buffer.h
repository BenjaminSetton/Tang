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

		void Create(VkDeviceSize size) override;

		void Destroy() override;

		void MapMemory(VkDeviceSize bufferSize);
		void UnMapMemory();

		void UpdateData(void* data, uint32_t numBytes);

		void* GetMappedData();

	private:

		void* mappedData;
	};
}

#endif