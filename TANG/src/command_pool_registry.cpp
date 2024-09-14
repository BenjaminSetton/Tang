
#include "command_pool_registry.h"
#include "device_cache.h"
#include "utils/logger.h"
#include "utils/sanity_check.h"

namespace TANG
{
	CommandPoolRegistry::CommandPoolRegistry() : pools()
	{
	}

	void CommandPoolRegistry::CreatePools(VkSurfaceKHR surface)
	{
		// Ensure a new pool is created after a new queue type is added!
		TNG_ASSERT_COMPILE(static_cast<uint32_t>(QUEUE_TYPE::COUNT) == 4);

		VkPhysicalDevice physicalDevice = GetPhysicalDevice();

		QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice, surface);

		CreatePool_Helper(queueFamilyIndices, QUEUE_TYPE::GRAPHICS, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		CreatePool_Helper(queueFamilyIndices, QUEUE_TYPE::COMPUTE, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		CreatePool_Helper(queueFamilyIndices, QUEUE_TYPE::TRANSFER, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	}

	void CommandPoolRegistry::DestroyPools()
	{
		VkDevice logicalDevice = GetLogicalDevice();

		for (const auto& iter : pools)
		{
			vkDestroyCommandPool(logicalDevice, iter.second, nullptr);
		}
		pools.clear();
	}

	VkCommandPool CommandPoolRegistry::GetCommandPool(QUEUE_TYPE type) const
	{
		if (type == QUEUE_TYPE::COUNT) return VK_NULL_HANDLE;

		auto iter = pools.find(type);
		if (iter == pools.end()) return VK_NULL_HANDLE;

		return iter->second;
	}

	void CommandPoolRegistry::CreatePool_Helper(const QueueFamilyIndices& queueFamilyIndices, QUEUE_TYPE type, VkCommandPoolCreateFlags flags)
	{
		if (queueFamilyIndices.IsValid(static_cast<QueueFamilyIndices::QueueFamilyIndexType>(type)))
		{
			// Allocate the pool object in the map
			pools[type] = VkCommandPool();

			VkCommandPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.flags = flags;
			poolInfo.queueFamilyIndex = queueFamilyIndices.GetIndex(type);

			if (vkCreateCommandPool(GetLogicalDevice(), &poolInfo, nullptr, &pools[type]) != VK_SUCCESS)
			{
				LogError("Failed to create command pool of type %u!", static_cast<uint32_t>(type));
			}
		}
		else
		{
			LogError("Failed to find a queue family supporting a queue of type %u!", static_cast<uint32_t>(type));
		}
	}
}