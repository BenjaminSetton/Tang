
#include "../command_pool_registry.h"
#include "../device_cache.h"
#include "../utils/sanity_check.h"
#include "../utils/logger.h"
#include "secondary_command_buffer.h"

#include <utility> // std::move

namespace TANG
{
	COMMAND_BUFFER_TYPE SecondaryCommandBuffer::GetType()
	{
		return COMMAND_BUFFER_TYPE::SECONDARY;
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
}