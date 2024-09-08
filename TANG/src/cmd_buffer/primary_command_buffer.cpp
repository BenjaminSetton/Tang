
#include <array>

#include "../device_cache.h"
#include "../framebuffer.h"
#include "../render_passes/base_render_pass.h"
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

	void PrimaryCommandBuffer::CMD_BeginRenderPass(const BaseRenderPass* _renderPass, Framebuffer* _frameBuffer, VkExtent2D renderAreaExtent, bool usingSecondaryCmdBuffers, bool clearBuffers)
	{
		if (renderPassState == PRIMARY_COMMAND_RENDER_PASS_STATE::BEGUN)
		{
			LogWarning("Mismatched BeginRenderPass/EndRenderPass calls!");
			return;
		}

		if (!IsCommandBufferValid() || !IsRecording())
		{
			LogWarning("Failed to begin render pass! Primary command buffer is not recording or command buffer is null");
			return;
		}

		if (_renderPass == nullptr)
		{
			LogWarning("Attempting to begin render pass with an invalid render pass pointer!");
			return;
		}

		if (_frameBuffer == nullptr)
		{
			LogWarning("Attempting to begin render pass with an invalid framebuffer pointer!");
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
		renderPassInfo.renderPass = _renderPass->GetRenderPass();
		renderPassInfo.framebuffer = _frameBuffer->GetFramebuffer();
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

		// Cache the framebuffer so we can transition the layout of the framebuffer attachments once the render pass ends
		renderPass = _renderPass;
		framebuffer = _frameBuffer;
	}

	void PrimaryCommandBuffer::CMD_EndRenderPass()
	{
		if (renderPassState == PRIMARY_COMMAND_RENDER_PASS_STATE::ENDED)
		{
			LogWarning("Mismatched BeginRenderPass/EndRenderPass calls!");
			return;
		}

		if (!IsCommandBufferValid() || !IsRecording())
		{
			LogWarning("Failed to end render pass! Primary command buffer is not recording or command buffer is null");
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
		if (!IsCommandBufferValid() || !IsRecording())
		{
			LogWarning("Failed to start next subpass! Primary command buffer is not recording or command buffer is null");
			return;
		}

		VkSubpassContents subpassContents = usingSecondaryCmdBuffers ? VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : VK_SUBPASS_CONTENTS_INLINE;
		vkCmdNextSubpass(commandBuffer, subpassContents);
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

	COMMAND_BUFFER_TYPE PrimaryCommandBuffer::GetType()
	{
		return COMMAND_BUFFER_TYPE::PRIMARY;
	}
}