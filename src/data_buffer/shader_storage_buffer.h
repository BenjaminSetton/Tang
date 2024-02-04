#ifndef SHADER_STORAGE_BUFFER_H
#define SHADER_STORAGE_BUFFER_H

#include "buffer.h"

namespace TANG
{
	class ShaderStorageBuffer : public Buffer
	{
	public:

		// Defines any usage for this buffer other than the mandatory STORAGE_BUFFER_BIT and TRANSFER_DST_BIT
		ShaderStorageBuffer(VkBufferUsageFlags extraUsage = 0);
		~ShaderStorageBuffer();
		ShaderStorageBuffer(const ShaderStorageBuffer& other);
		ShaderStorageBuffer(ShaderStorageBuffer&& other) noexcept;
		ShaderStorageBuffer& operator=(const ShaderStorageBuffer& other);

		void Create(VkDeviceSize size) override;
		void Destroy() override;

	private:

		VkBufferUsageFlags extraUsage;
		
	};
}


#endif