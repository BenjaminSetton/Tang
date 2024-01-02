
#include <array>

#include "../device_cache.h"
#include "../utils/sanity_check.h"
#include "../utils/logger.h"
#include "primary_command_buffer.h"

namespace TANG
{

	PrimaryCommandBuffer::PrimaryCommandBuffer() : renderPassState(PRIMARY_COMMAND_RENDER_PASS_STATE::ENDED)
	{
	}

	PrimaryCommandBuffer::~PrimaryCommandBuffer()
	{
		// Nothing to do here
	}

	PrimaryCommandBuffer::PrimaryCommandBuffer(const PrimaryCommandBuffer& other) : CommandBuffer(other)
	{
		if (other.renderPassState == PRIMARY_COMMAND_RENDER_PASS_STATE::BEGUN)
		{
			TNG_ASSERT_MSG(false, "Why are we copying a primary command buffer while recording a render pass?");
		}

		renderPassState = other.renderPassState;
	}

	PrimaryCommandBuffer::PrimaryCommandBuffer(PrimaryCommandBuffer&& other) noexcept : CommandBuffer(std::move(other))
	{
		if (other.renderPassState == PRIMARY_COMMAND_RENDER_PASS_STATE::BEGUN)
		{
			TNG_ASSERT_MSG(false, "Why are we moving a primary command buffer while recording a render pass?");
		}

		renderPassState = other.renderPassState;
	}

	PrimaryCommandBuffer& PrimaryCommandBuffer::operator=(const PrimaryCommandBuffer& other)
	{
		if (other.renderPassState == PRIMARY_COMMAND_RENDER_PASS_STATE::BEGUN)
		{
			TNG_ASSERT_MSG(false, "Why are we copying a primary command buffer while recording a render pass?");
		}

		// Protect against self-assignment
		if (this == &other)
		{
			return *this;
		}

		CommandBuffer::operator=(other);
		renderPassState = other.renderPassState;
		return *this;
	}

	void PrimaryCommandBuffer::Create(VkCommandPool commandPool)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(GetLogicalDevice(), &allocInfo, &commandBuffer) != VK_SUCCESS) {
			TNG_ASSERT_MSG(false, "Failed to allocate primary command buffer!");
		}

		cmdBufferState = COMMAND_BUFFER_STATE::ALLOCATED;
	}

	//void PrimaryCommandBuffer::SubmitToQueue(VkQueue queue)
	//{
	//	if (!IsCommandBufferValid() || IsRecording())
	//	{
	//		LogWarning("Failed to submit primary command buffer to queue. Primary command buffer is still recording or command buffer is null");
	//		return;
	//	}

	//	VkSubmitInfo submitInfo = {};
	//	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	//	submitInfo.commandBufferCount = 1;
	//	submitInfo.pCommandBuffers = &commandBuffer;
	//	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	//	vkQueueWaitIdle(queue);
	//}

	void PrimaryCommandBuffer::CMD_BeginRenderPass(VkRenderPass renderPass, VkFramebuffer frameBuffer, VkExtent2D renderAreaExtent, bool usingSecondaryCmdBuffers, bool clearBuffers)
	{
		if (!IsCommandBufferValid() || !IsRecording())
		{
			LogWarning("Failed to begin render pass! Primary command buffer is not recording or command buffer is null");
			return;
		}

		if (renderAreaExtent.width == 0.0f || renderAreaExtent.height == 0.0f)
		{
			LogWarning("Render area width or height is set to zero for render pass begin! Loading the contents of the previous frame will not work as expected");
		}

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { { 0.64f, 0.8f, 0.76f, 1.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = frameBuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = renderAreaExtent;

		if (clearBuffers)
		{
			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();
		}
		else
		{
			renderPassInfo.clearValueCount = 0;
			renderPassInfo.pClearValues = nullptr;
		}

		VkSubpassContents subpassContents = usingSecondaryCmdBuffers ? VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : VK_SUBPASS_CONTENTS_INLINE;
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, subpassContents);

		renderPassState = PRIMARY_COMMAND_RENDER_PASS_STATE::BEGUN;
	}

	void PrimaryCommandBuffer::CMD_EndRenderPass()
	{
		if (!IsCommandBufferValid() || !IsRecording())
		{
			LogWarning("Failed to end render pass! Primary command buffer is not recording or command buffer is null");
			return;
		}

		vkCmdEndRenderPass(commandBuffer);

		renderPassState = PRIMARY_COMMAND_RENDER_PASS_STATE::ENDED;
	}

	void PrimaryCommandBuffer::CMD_ExecuteSecondaryCommands(VkCommandBuffer* cmdBuffers, uint32_t cmdBufferCount)
	{
		if (!IsCommandBufferValid() || !IsRecording())
		{
			LogWarning("Failed to bind execute command! Primary command buffer is not recording or command buffer is null");
			return;
		}

		vkCmdExecuteCommands(commandBuffer, cmdBufferCount, cmdBuffers);
	}
}