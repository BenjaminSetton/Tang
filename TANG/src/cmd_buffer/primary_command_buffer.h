#ifndef PRIMARY_COMMAND_BUFFER_H
#define PRIMARY_COMMAND_BUFFER_H

#include "command_buffer.h"

namespace TANG
{
	// Forward declarations
	class BaseRenderPass;
	class Framebuffer;

	enum class PRIMARY_COMMAND_RENDER_PASS_STATE
	{
		BEGUN,
		ENDED
	};

	// Encapsulates the functionality for a primary command buffer
	class PrimaryCommandBuffer : public CommandBuffer
	{
	public:

		friend class Renderer;

		PrimaryCommandBuffer();
		~PrimaryCommandBuffer();
		PrimaryCommandBuffer(const PrimaryCommandBuffer& other);
		PrimaryCommandBuffer& operator=(const PrimaryCommandBuffer& other);
		PrimaryCommandBuffer(PrimaryCommandBuffer&& other) noexcept;

		// Begin and end commands for a render pass
		void CMD_BeginRenderPass(const BaseRenderPass* renderPass, const Framebuffer* frameBuffer, VkExtent2D renderAreaExtent, bool usingSecondaryCmdBuffers, bool clearBuffers);
		void CMD_EndRenderPass(const BaseRenderPass* renderPass, Framebuffer* framebuffer);

		void CMD_NextSubpass(bool usingSecondaryCmdBuffers);

		// Records an execution command for the provided secondary command buffer
		void CMD_ExecuteSecondaryCommands(VkCommandBuffer* cmdBuffers, uint32_t cmdBufferCount);

		COMMAND_BUFFER_TYPE GetType() override;

	protected:

		void Allocate(QUEUE_TYPE type) override;

	private:

		// The renderPassState is initialized to ENDED. This sounds weird, but the only state that can transition to BEGUN is ENDED,
		// which is the first state-altering function that should be called (e.g. EndRenderPass() shouldn't be called before BeginRenderPass())
		PRIMARY_COMMAND_RENDER_PASS_STATE renderPassState;
	};
}

#endif