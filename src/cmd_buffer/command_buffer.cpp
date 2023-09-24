
#include "command_buffer.h"
#include "../utils/logger.h"
#include "../utils/sanity_check.h"

namespace TANG
{

	CommandBuffer::CommandBuffer() : commandBuffer(VK_NULL_HANDLE), cmdBufferState(COMMAND_BUFFER_STATE::DEFAULT), isOneTimeSubmit(false)
	{
	}

	CommandBuffer::~CommandBuffer()
	{
		if (cmdBufferState == COMMAND_BUFFER_STATE::RECORDING)
		{
			LogWarning("The command buffer handle is being lost while the command buffer is in the recording state!");
		}
	}

	CommandBuffer::CommandBuffer(const CommandBuffer& other)
	{
		if (IsRecording() || other.IsRecording())
		{
			TNG_ASSERT_MSG(false, "Cannot copy a command buffer while either one is recording!");
		}

		commandBuffer = other.commandBuffer;
		cmdBufferState = other.cmdBufferState;
		isOneTimeSubmit = other.isOneTimeSubmit;
	}

	CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept
	{
		commandBuffer = other.commandBuffer;
		cmdBufferState = other.cmdBufferState;
		isOneTimeSubmit = other.isOneTimeSubmit;

		// Transfer ownership of the other command buffers' internal buffer to this object, and set it's handle to null
		other.commandBuffer = VK_NULL_HANDLE;
	}

	CommandBuffer& CommandBuffer::operator=(const CommandBuffer& other)
	{
		if (IsRecording() || other.IsRecording())
		{
			TNG_ASSERT_MSG(false, "Cannot copy a command buffer while either one is recording!");
		}

		// Protect against self-assignment
		if (this == &other)
		{
			return *this;
		}

		commandBuffer = other.commandBuffer;
		cmdBufferState = other.cmdBufferState;
		isOneTimeSubmit = other.isOneTimeSubmit;
		return *this;
	}

	void CommandBuffer::Destroy(VkDevice logicalDevice, VkCommandPool commandPool)
	{
		if (!IsCommandBufferValid() || IsRecording())
		{
			LogError("Can't destroy a command buffer because it's either still recording or the command buffer is null! Memory will be leaked");
			return;
		}

		vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
		commandBuffer = VK_NULL_HANDLE;

		cmdBufferState = COMMAND_BUFFER_STATE::DESTROYED;
	}

	void CommandBuffer::BeginRecording(VkCommandBufferUsageFlags flags, VkCommandBufferInheritanceInfo* inheritanceInfo)
	{
		if (!IsCommandBufferValid() || IsRecording())
		{
			LogWarning("Failed to begin recording. Cannot write commands to this command buffer");
			return;
		}

		if (isOneTimeSubmit && cmdBufferState != COMMAND_BUFFER_STATE::RESET)
		{
			LogWarning("One-time-submit command buffer has started recording, but has not been reset!");
			//return; // Do we want to return here?
		}

		VkCommandBufferBeginInfo beginInfo = {};
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
		isOneTimeSubmit = (flags & VkCommandBufferUsageFlagBits::VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) != 0;
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

	bool CommandBuffer::IsCommandBufferValid() const
	{
		return commandBuffer != VK_NULL_HANDLE && cmdBufferState != COMMAND_BUFFER_STATE::DESTROYED;
	}

}