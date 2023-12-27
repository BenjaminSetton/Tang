#ifndef PRIMARY_COMMAND_BUFFER_H
#define PRIMARY_COMMAND_BUFFER_H

#include "command_buffer.h"

namespace TANG
{

	enum class PRIMARY_COMMAND_RENDER_PASS_STATE
	{
		BEGUN,
		ENDED
	};

	// Encapsulates the functionality for a primary command buffer
	class PrimaryCommandBuffer : public CommandBuffer
	{
	public:

		PrimaryCommandBuffer();
		~PrimaryCommandBuffer();
		PrimaryCommandBuffer(const PrimaryCommandBuffer& other);
		PrimaryCommandBuffer(PrimaryCommandBuffer&& other) noexcept;
		PrimaryCommandBuffer& operator=(const PrimaryCommandBuffer& other);

		void Create(VkCommandPool commandPool) override;

		// Submits all currently written commands to the provided queue
		//void SubmitToQueue(VkQueue queue);

		// Begin and end commands for a render pass
		void CMD_BeginRenderPass(VkRenderPass renderPass, VkFramebuffer frameBuffer, VkExtent2D renderAreaExtent, bool usingSecondaryCmdBuffers);
		void CMD_EndRenderPass();

		// Records an execution command for the provided secondary command buffer
		void CMD_ExecuteSecondaryCommands(VkCommandBuffer* cmdBuffers, uint32_t cmdBufferCount);

	private:

		// The renderPassState is initialized to ENDED. This sounds weird, but the only state that can transition to BEGUN is ENDED,
		// which is the first state-altering function that should be called (e.g. EndRenderPass() shouldn't be called before BeginRenderPass())
		PRIMARY_COMMAND_RENDER_PASS_STATE renderPassState;
	};
}

#endif