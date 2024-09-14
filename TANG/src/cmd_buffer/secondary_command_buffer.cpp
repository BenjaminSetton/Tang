
#include "../command_pool_registry.h"
#include "../device_cache.h"
#include "../utils/sanity_check.h"
#include "../utils/logger.h"
#include "secondary_command_buffer.h"

#include <utility> // std::move

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

	void SecondaryCommandBuffer::Allocate(QUEUE_TYPE type)
	{
		VkDevice logicalDevice = GetLogicalDevice();

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		allocInfo.commandPool = GetCommandPool(type);
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer) != VK_SUCCESS) {
			TNG_ASSERT_MSG(false, "Failed to allocate secondary command buffer!");
		}

		cmdBufferState = COMMAND_BUFFER_STATE::ALLOCATED;
		allocatedQueueType = type;
	}

	COMMAND_BUFFER_TYPE SecondaryCommandBuffer::GetType()
	{
		return COMMAND_BUFFER_TYPE::SECONDARY;
	}
}