
#include "../asset_types.h" // AssetResources
#include "../command_pool_registry.h" // GetCommandPool()
#include "../data_buffer/index_buffer.h" // GetIndexType()
#include "../device_cache.h"
#include "../pipelines/base_pipeline.h" // BasePipeline
#include "../utils/logger.h"
#include "../utils/sanity_check.h"
#include "command_buffer.h"

namespace TANG
{

	CommandBuffer::CommandBuffer() : commandBuffer(VK_NULL_HANDLE), cmdBufferState(COMMAND_BUFFER_STATE::DEFAULT), allocatedQueueType(QUEUE_TYPE::COUNT)
	{
	}

	CommandBuffer::~CommandBuffer()
	{
		if (cmdBufferState == COMMAND_BUFFER_STATE::RECORDING)
		{
			LogWarning("The command buffer handle is being lost while the command buffer is in the recording state!");
		}
	}

	CommandBuffer::CommandBuffer(const CommandBuffer& other) : commandBuffer(other.commandBuffer), 
		cmdBufferState(other.cmdBufferState), allocatedQueueType(other.allocatedQueueType)
	{
	}

	CommandBuffer& CommandBuffer::operator=(const CommandBuffer& other)
	{
		if (this == &other)
		{
			return *this;
		}

		commandBuffer = other.commandBuffer;
		cmdBufferState = other.cmdBufferState;
		allocatedQueueType = other.allocatedQueueType;
		return *this;
	}

	CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept
	{
		commandBuffer = other.commandBuffer;
		cmdBufferState = other.cmdBufferState;
		allocatedQueueType = other.allocatedQueueType;

		// Transfer ownership of the other command buffers' internal buffer to this object, and set it's handle to null
		other.commandBuffer = VK_NULL_HANDLE;
		other.cmdBufferState = COMMAND_BUFFER_STATE::DEFAULT;
	}

	void CommandBuffer::BeginRecording(VkCommandBufferUsageFlags flags, VkCommandBufferInheritanceInfo* inheritanceInfo)
	{
		if (!IsValid() || IsRecording())
		{
			LogWarning("Failed to begin recording. Cannot write commands to this command buffer");
			return;
		}

		if (IsOneTimeSubmit(flags) && !IsWritable())
		{
			LogWarning("One-time-submit command buffer has started recording, but is not writable! Current state is %u", cmdBufferState);
			return;
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = flags;
		beginInfo.pInheritanceInfo = inheritanceInfo;
		VkResult res = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		if (res != VK_SUCCESS)
		{
			LogWarning("Failed to start recording command buffer! Got error code: %i", static_cast<uint32_t>(res));
			return;
		}

		cmdBufferState = COMMAND_BUFFER_STATE::RECORDING;
	}

	void CommandBuffer::EndRecording()
	{
		if (cmdBufferState == COMMAND_BUFFER_STATE::SEALED)
		{
			LogWarning("Can't end recording on a buffer that's not currently being recorded. Either BeginRecording() was not called or there were multiple EndRecording() calls");
			return;
		}

		vkEndCommandBuffer(commandBuffer);

		cmdBufferState = COMMAND_BUFFER_STATE::SEALED;
	}

	void CommandBuffer::CMD_BindMesh(const AssetResources* resources)
	{
		if (!IsValid() || !IsRecording() || (resources == nullptr))
		{
			LogWarning("Failed to bind mesh! Command buffer is not recording");
			return;
		}

		VkBuffer vertexBuffer = resources->vertexBuffer.GetBuffer();
		VkDeviceSize offset = resources->offset;

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
		vkCmdBindIndexBuffer(commandBuffer, resources->indexBuffer.GetBuffer(), 0, resources->indexBuffer.GetIndexType());
	}

	void CommandBuffer::CMD_BindDescriptorSets(const BasePipeline* pipeline, uint32_t descriptorSetCount, DescriptorSet* descriptorSets)
	{
		if (!IsValid() || !IsRecording())
		{
			LogWarning("Failed to bind descriptor sets! Command buffer is not recording");
			return;
		}

		// This explicit cast to a VkDescriptorSet only works because we have a static assert that won't let the program compile unless sizeof(VkDescriptorSet) == sizeof(DescriptorSet )
		vkCmdBindDescriptorSets(commandBuffer, pipeline->GetBindPoint(), pipeline->GetPipelineLayout(), 0, descriptorSetCount, reinterpret_cast<VkDescriptorSet*>(descriptorSets), 0, nullptr);
	}

	void CommandBuffer::CMD_PushConstants(const BasePipeline* pipeline, void* constantData, uint32_t size, VkShaderStageFlags stageFlags)
	{
		if (!IsValid() || !IsRecording())
		{
			LogWarning("Failed to bind descriptor sets! Command buffer is not recording");
			return;
		}

		vkCmdPushConstants(commandBuffer, pipeline->GetPipelineLayout(), stageFlags, 0, size, constantData);
	}

	void CommandBuffer::CMD_BindPipeline(const BasePipeline* pipeline)
	{
		if (!IsValid() || !IsRecording())
		{
			LogWarning("Failed to bind pipeline command! Command buffer is not recording or command buffer is null");
			return;
		}

		vkCmdBindPipeline(commandBuffer, pipeline->GetBindPoint(), pipeline->GetPipeline());
	}

	void CommandBuffer::CMD_SetViewport(float width, float height)
	{
		if (!IsValid() || !IsRecording())
		{
			LogWarning("Failed to bind set-viewport command! Command buffer is not recording or command buffer is null");
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

	void CommandBuffer::CMD_SetScissor(VkOffset2D scissorOffset, VkExtent2D scissorExtent)
	{
		if (!IsValid() || !IsRecording())
		{
			LogWarning("Failed to bind set-scissor command! Command buffer is not recording or command buffer is null");
			return;
		}

		VkRect2D scissor{};
		scissor.offset = scissorOffset;
		scissor.extent = scissorExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void CommandBuffer::CMD_Draw(uint32_t vertexCount)
	{
		if (!IsValid() || !IsRecording())
		{
			LogWarning("Failed to bind draw command! Command buffer is not recording");
			return;
		}

		vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
	}

	void CommandBuffer::CMD_DrawIndexed(uint64_t indexCount)
	{
		if (!IsValid() || !IsRecording())
		{
			LogWarning("Failed to bind draw indexed command! Command buffer is not recording");
			return;
		}

		if (indexCount > std::numeric_limits<uint32_t>::max())
		{
			LogError("Index count in draw indexed call exceeds uint32_t::max allowed by Vulkan API call! Only a portion of the mesh will be rendered");
		}

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indexCount), 1, 0, 0, 0);
	}

	void CommandBuffer::CMD_DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount)
	{
		if (!IsValid() || !IsRecording())
		{
			LogWarning("Failed to bind draw indexed instanced command! Command buffer is not recording");
			return;
		}

		vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
	}

	void CommandBuffer::CMD_Dispatch(uint32_t x, uint32_t y, uint32_t z)
	{
		if (!IsValid() || !IsRecording())
		{
			LogWarning("Failed to bind dispatch command! Command buffer is not recording");
			return;
		}

		vkCmdDispatch(commandBuffer, x, y, z);
	}

	void CommandBuffer::Reset(bool releaseMemory)
	{
		vkResetCommandBuffer(commandBuffer, releaseMemory ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT : 0);

		cmdBufferState = COMMAND_BUFFER_STATE::RESET;
	}

	VkCommandBuffer CommandBuffer::GetBuffer() const
	{
		return commandBuffer;
	}

	bool CommandBuffer::IsRecording() const
	{
		return cmdBufferState == COMMAND_BUFFER_STATE::RECORDING;
	}

	bool CommandBuffer::IsReset() const
	{
		return cmdBufferState == COMMAND_BUFFER_STATE::RESET;
	}

	bool CommandBuffer::IsAllocated() const
	{
		return (cmdBufferState != COMMAND_BUFFER_STATE::DEFAULT) && (cmdBufferState != COMMAND_BUFFER_STATE::DESTROYED);
	}

	bool CommandBuffer::IsWritable() const
	{
		return (cmdBufferState == COMMAND_BUFFER_STATE::RESET) || (cmdBufferState == COMMAND_BUFFER_STATE::ALLOCATED);
	}

	bool CommandBuffer::IsValid() const
	{
		return commandBuffer != VK_NULL_HANDLE && cmdBufferState != COMMAND_BUFFER_STATE::DESTROYED;
	}

	QUEUE_TYPE CommandBuffer::GetAllocatedQueueType() const
	{
		TNG_ASSERT(allocatedQueueType != QUEUE_TYPE::COUNT);
		return allocatedQueueType;
	}

	void CommandBuffer::Destroy()
	{
		VkDevice logicalDevice = GetLogicalDevice();

		if (!IsValid() || IsRecording() || !IsAllocated())
		{
			LogError("Can't destroy a command buffer because it's either still recording or the command buffer is null! Potential memory leak!");
			return;
		}

		vkFreeCommandBuffers(logicalDevice, GetCommandPool(allocatedQueueType), 1, &commandBuffer);
		commandBuffer = VK_NULL_HANDLE;

		cmdBufferState = COMMAND_BUFFER_STATE::DESTROYED;
	}

	bool CommandBuffer::IsOneTimeSubmit(VkCommandBufferUsageFlags usageFlags) const
	{
		return (usageFlags & VkCommandBufferUsageFlagBits::VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) != 0;
	}
}