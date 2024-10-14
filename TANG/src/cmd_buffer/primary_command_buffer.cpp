
#include <array>

#include "../command_pool_registry.h"
#include "../device_cache.h"
#include "../framebuffer.h"
#include "../render_pass/base_render_pass.h"
#include "../texture_resource.h"
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
	}

	PrimaryCommandBuffer::PrimaryCommandBuffer(const PrimaryCommandBuffer& other) : CommandBuffer(other), renderPassState(other.renderPassState)
	{
	}

	PrimaryCommandBuffer& PrimaryCommandBuffer::operator=(const PrimaryCommandBuffer& other)
	{
		if (this == &other)
		{
			return *this;
		}

		// Call operator= on parent?
		renderPassState = other.renderPassState;
		return *this;
	}

	PrimaryCommandBuffer::PrimaryCommandBuffer(PrimaryCommandBuffer&& other) noexcept : CommandBuffer(std::move(other))
	{
		if (other.renderPassState == PRIMARY_COMMAND_RENDER_PASS_STATE::BEGUN)
		{
			TNG_ASSERT_MSG(false, "Why are we moving a primary command buffer while recording a render pass?");
		}

		renderPassState = other.renderPassState;
	}

	void PrimaryCommandBuffer::CMD_BeginRenderPass(const BaseRenderPass* renderPass, const Framebuffer* framebuffer, bool usingSecondaryCmdBuffers, bool clearBuffers)
	{
		if (renderPassState == PRIMARY_COMMAND_RENDER_PASS_STATE::BEGUN)
		{
			LogError("Mismatched BeginRenderPass/EndRenderPass calls!");
			return;
		}

		if (!IsValid() || !IsRecording())
		{
			LogError("Failed to begin render pass! Primary command buffer is not recording or command buffer is null");
			return;
		}

		if (renderPass == nullptr)
		{
			LogError("Attempting to begin render pass with an invalid render pass pointer!");
			return;
		}

		if (framebuffer == nullptr)
		{
			LogError("Attempting to begin render pass with an invalid framebuffer pointer!");
			return;
		}

		//if (renderAreaExtent.width == 0.0f || renderAreaExtent.height == 0.0f)
		//{
		//	LogWarning("Render area width or height is set to zero for render pass begin! Loading the contents of the previous frame will not work as expected");
		//}

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { { 0.64f, 0.8f, 0.76f, 1.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass->GetRenderPass();
		renderPassInfo.framebuffer = framebuffer->GetFramebuffer();
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = { framebuffer->GetWidth(), framebuffer->GetHeight() };

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

	void PrimaryCommandBuffer::CMD_EndRenderPass(const BaseRenderPass* renderPass, Framebuffer* framebuffer)
	{
		if (renderPassState == PRIMARY_COMMAND_RENDER_PASS_STATE::ENDED)
		{
			LogError("Mismatched BeginRenderPass/EndRenderPass calls!");
			return;
		}

		if (!IsValid() || !IsRecording())
		{
			LogError("Failed to end render pass! Primary command buffer is not recording or command buffer is null");
			return;
		}

		if (renderPass == nullptr)
		{
			LogError("Attempting to begin render pass with an invalid render pass pointer!");
			return;
		}

		if (framebuffer == nullptr)
		{
			LogError("Attempting to begin render pass with an invalid framebuffer pointer!");
			return;
		}

		vkCmdEndRenderPass(commandBuffer);

		renderPassState = PRIMARY_COMMAND_RENDER_PASS_STATE::ENDED;

		// Forcefully transition the attachments
		std::vector<TextureResource*> attachmentImages = framebuffer->GetAttachmentImages();
		const VkImageLayout* finalImageLayouts = renderPass->GetFinalImageLayouts();
		for (uint32_t i = 0; i < attachmentImages.size(); i++)
		{
			attachmentImages[i]->TransitionLayout_Force(finalImageLayouts[i]);
		}
	}

	void PrimaryCommandBuffer::CMD_NextSubpass(bool usingSecondaryCmdBuffers)
	{
		if (!IsValid() || !IsRecording())
		{
			LogWarning("Failed to start next subpass! Primary command buffer is not recording or command buffer is null");
			return;
		}

		VkSubpassContents subpassContents = usingSecondaryCmdBuffers ? VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : VK_SUBPASS_CONTENTS_INLINE;
		vkCmdNextSubpass(commandBuffer, subpassContents);
	}

	void PrimaryCommandBuffer::CMD_ExecuteSecondaryCommands(VkCommandBuffer* cmdBuffers, uint32_t cmdBufferCount)
	{
		if (!IsValid() || !IsRecording())
		{
			LogWarning("Failed to bind execute command! Primary command buffer is not recording or command buffer is null");
			return;
		}

		vkCmdExecuteCommands(commandBuffer, cmdBufferCount, cmdBuffers);
	}

	COMMAND_BUFFER_TYPE PrimaryCommandBuffer::GetType()
	{
		return COMMAND_BUFFER_TYPE::PRIMARY;
	}

	void PrimaryCommandBuffer::Allocate(QUEUE_TYPE type)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = GetCommandPool(type);
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(GetLogicalDevice(), &allocInfo, &commandBuffer) != VK_SUCCESS) {
			TNG_ASSERT_MSG(false, "Failed to allocate primary command buffer!");
		}

		cmdBufferState = COMMAND_BUFFER_STATE::ALLOCATED;
		allocatedQueueType = type;
	}
}