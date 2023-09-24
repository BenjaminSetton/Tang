
#include "../asset_types.h"
#include "../data_buffer/index_buffer.h" // For GetIndexType()
#include "secondary_command_buffer.h"
#include "../utils/sanity_check.h"
#include "../utils/logger.h"

namespace TANG
{

	SecondaryCommandBuffer::SecondaryCommandBuffer()
	{
		// Nothing to do here
	}

	SecondaryCommandBuffer::~SecondaryCommandBuffer()
	{
		// Nothing to do here
	}

	SecondaryCommandBuffer::SecondaryCommandBuffer(const SecondaryCommandBuffer& other) : CommandBuffer(other)
	{
		// All we need to do here is call the copy constructor of the parent
	}

	SecondaryCommandBuffer::SecondaryCommandBuffer(SecondaryCommandBuffer&& other) noexcept : CommandBuffer(std::move(other))
	{
		// All we need to do here is call the move constructor of the parent
	}

	SecondaryCommandBuffer& SecondaryCommandBuffer::operator=(const SecondaryCommandBuffer& other)
	{
		if (this == &other)
		{
			return *this;
		}

		// All we need to do here is call the assignment operator of the parent
		CommandBuffer::operator=(other);
		return *this;
	}

	void SecondaryCommandBuffer::Create(VkDevice logicalDevice, VkCommandPool commandPool)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer) != VK_SUCCESS) {
			TNG_ASSERT_MSG(false, "Failed to allocate secondary command buffer!");
		}

		cmdBufferState = COMMAND_BUFFER_STATE::ALLOCATED;
	}

	void SecondaryCommandBuffer::CMD_BindMesh(AssetResources* resources)
	{
		if (!IsCommandBufferValid() || !IsRecording())
		{
			LogWarning("Failed to bind mesh! Secondary command buffer is not recording");
			return;
		}

		uint32_t numVertexBuffers = static_cast<uint32_t>(resources->vertexBuffers.size());

		std::vector<VkBuffer> vertexBuffers(numVertexBuffers);
		std::vector<VkDeviceSize> offsets(numVertexBuffers);
		for (uint32_t i = 0; i < numVertexBuffers; i++)
		{
			vertexBuffers[i] = resources->vertexBuffers[i].GetBuffer();
			offsets[i] = resources->offsets[i]; // Conversion from uint32_t to uint64_t!
		}

		vkCmdBindVertexBuffers(commandBuffer, 0, numVertexBuffers, vertexBuffers.data(), offsets.data());
		vkCmdBindIndexBuffer(commandBuffer, resources->indexBuffer.GetBuffer(), 0, resources->indexBuffer.GetIndexType());
	}

	void SecondaryCommandBuffer::CMD_BindDescriptorSets(VkPipelineLayout pipelineLayout, uint32_t descriptorSetCount, VkDescriptorSet* descriptorSets)
	{
		if (!IsCommandBufferValid() || !IsRecording())
		{
			LogWarning("Failed to bind descriptor sets! Secondary command buffer is not recording");
			return;
		}

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, descriptorSetCount, descriptorSets, 0, nullptr);
	}

	void SecondaryCommandBuffer::CMD_BindGraphicsPipeline(VkPipeline graphicsPipeline)
	{
		if (!IsCommandBufferValid() || !IsRecording())
		{
			LogWarning("Failed to bind pipeline command! Primary command buffer is not recording or command buffer is null");
			return;
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	}

	void SecondaryCommandBuffer::CMD_SetViewport(float width, float height)
	{
		if (!IsCommandBufferValid() || !IsRecording())
		{
			LogWarning("Failed to bind set-viewport command! Primary command buffer is not recording or command buffer is null");
			return;
		}

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = width;
		viewport.height = height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	}

	void SecondaryCommandBuffer::CMD_SetScissor(VkOffset2D scissorOffset, VkExtent2D scissorExtent)
	{
		if (!IsCommandBufferValid() || !IsRecording())
		{
			LogWarning("Failed to bind set-scissor command! Primary command buffer is not recording or command buffer is null");
			return;
		}

		VkRect2D scissor{};
		scissor.offset = scissorOffset;
		scissor.extent = scissorExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void SecondaryCommandBuffer::CMD_Draw(uint32_t vertexCount)
	{
		if (!IsCommandBufferValid() || !IsRecording())
		{
			LogWarning("Failed to bind draw command! Secondary command buffer is not recording");
			return;
		}

		vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
	}

	void SecondaryCommandBuffer::CMD_DrawIndexed(uint32_t indexCount)
	{
		if (!IsCommandBufferValid() || !IsRecording())
		{
			LogWarning("Failed to bind draw indexed command! Secondary command buffer is not recording");
			return;
		}

		vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
	}

	void SecondaryCommandBuffer::CMD_DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount)
	{
		if (!IsCommandBufferValid() || !IsRecording())
		{
			LogWarning("Failed to bind draw indexed instanced command! Secondary command buffer is not recording");
			return;
		}

		vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
	}
}