#ifndef SECONDARY_COMMAND_BUFFER_H
#define SECONDARY_COMMAND_BUFFER_H

#include "command_buffer.h"

namespace TANG
{
	class SecondaryCommandBuffer : public CommandBuffer
	{
	public:

		friend class Renderer;

		SecondaryCommandBuffer() = default;
		~SecondaryCommandBuffer() = default;
		SecondaryCommandBuffer(const SecondaryCommandBuffer& other) = default;
		SecondaryCommandBuffer& operator=(const SecondaryCommandBuffer& other) = default;
		SecondaryCommandBuffer(SecondaryCommandBuffer&& other) noexcept = default;

		COMMAND_BUFFER_TYPE GetType() override;

	protected:

		void Allocate(QUEUE_TYPE type) override;

	};
}

#endif