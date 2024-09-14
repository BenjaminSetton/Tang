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
		SecondaryCommandBuffer(SecondaryCommandBuffer&& other) noexcept;

		void Allocate(QUEUE_TYPE type) override;

		COMMAND_BUFFER_TYPE GetType() override;

	};
}

#endif