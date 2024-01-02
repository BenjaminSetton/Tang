
#include "../command_pool_registry.h"
#include "disposable_command.h"
#include "../device_cache.h"
#include "../renderer.h"
#include "../utils/logger.h"

namespace TANG
{
	DisposableCommand::DisposableCommand(QueueType _type, bool _waitUntilQueueIdle) : type(_type), waitUntilQueueIdle(_waitUntilQueueIdle)
	{
		VkDevice logicalDevice = GetLogicalDevice();

		VkResult res;
		allocatedBuffer = VK_NULL_HANDLE;

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = CommandPoolRegistry::Get().GetCommandPool(type);
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		res = vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);
		if (res != VK_SUCCESS)
		{
			LogError("Failed to allocate disposable command buffer!");
			return;
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		res = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		if (res != VK_SUCCESS)
		{
			LogError("Failed to begin disposable command buffer!");
			return;
		}

		allocatedBuffer = commandBuffer;
		logicalDeviceHandle = logicalDevice;
	}

	DisposableCommand::~DisposableCommand()
	{
		if (allocatedBuffer == VK_NULL_HANDLE)
		{
			return;
		}

		vkEndCommandBuffer(allocatedBuffer);

		VkResult res;
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &allocatedBuffer;

		res = Renderer::GetInstance().SubmitQueue(type, &submitInfo, 1, VK_NULL_HANDLE, waitUntilQueueIdle);

		vkFreeCommandBuffers(logicalDeviceHandle, CommandPoolRegistry::Get().GetCommandPool(type), 1, &allocatedBuffer);
	}

	VkCommandBuffer DisposableCommand::GetBuffer() const
	{
		return allocatedBuffer;
	}
}