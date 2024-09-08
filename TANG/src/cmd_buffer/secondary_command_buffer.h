#ifndef SECONDARY_COMMAND_BUFFER_H
#define SECONDARY_COMMAND_BUFFER_H

#include "command_buffer.h"

namespace TANG
{
	class SecondaryCommandBuffer : public CommandBuffer
	{
	public:

		SecondaryCommandBuffer();
		~SecondaryCommandBuffer();
		SecondaryCommandBuffer(const SecondaryCommandBuffer& other);
		SecondaryCommandBuffer(SecondaryCommandBuffer&& other) noexcept;
		SecondaryCommandBuffer& operator=(const SecondaryCommandBuffer& other);

		void Create(VkCommandPool commandPool) override;

		COMMAND_BUFFER_TYPE GetType() override;

	};
}

#endif