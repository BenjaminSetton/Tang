
#include "command_pool_registry.h"
#include "utils/logger.h"
#include "utils/sanity_check.h"

namespace TANG
{
	void CommandPoolRegistry::CreatePools(VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
	{
		QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);

		// Allocate the graphics command pool
		if (queueFamilyIndices.IsValid(static_cast<QueueFamilyIndices::QueueFamilyIndexType>(QueueType::GRAPHICS)))
		{
			// Allocate the pool object in the map
			pools[QueueType::GRAPHICS] = VkCommandPool();

			VkCommandPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			poolInfo.queueFamilyIndex = queueFamilyIndices.queueFamilies[QueueType::GRAPHICS];

			if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &pools[QueueType::GRAPHICS]) != VK_SUCCESS)
			{
				LogError("Failed to create a graphics command pool!");
			}
		}
		else
		{
			TNG_ASSERT_MSG(false, "Failed to find a queue family supporting a graphics queue!");
		}

		// Allocate the transfer command pool
		if (queueFamilyIndices.IsValid(static_cast<QueueFamilyIndices::QueueFamilyIndexType>(QueueType::TRANSFER)))
		{
			pools[QueueType::TRANSFER] = VkCommandPool();

			VkCommandPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			// We support short-lived operations (transient bit) and we want to reset the command buffers after submissions
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			poolInfo.queueFamilyIndex = queueFamilyIndices.queueFamilies[QueueType::TRANSFER];

			if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &pools[QueueType::TRANSFER]) != VK_SUCCESS)
			{
				TNG_ASSERT_MSG(false, "Failed to create a transfer command pool!");
			}
		}
		else
		{
			TNG_ASSERT_MSG(false, "Failed to find a queue family supporting a transfer queue!");
		}
	}

	void CommandPoolRegistry::DestroyPools(VkDevice logicalDevice)
	{
		for (const auto& iter : pools)
		{
			vkDestroyCommandPool(logicalDevice, iter.second, nullptr);
		}
		pools.clear();
	}

	VkCommandPool CommandPoolRegistry::GetCommandPool(QueueType type) const
	{
		return pools[type];
	}

}