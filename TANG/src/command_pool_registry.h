#ifndef COMMAND_POOL_REGISTRY_H
#define COMMAND_POOL_REGISTRY_H

#include <unordered_map>

#include "queue_family_indices.h"
#include "queue_types.h"
#include "vulkan/vulkan.h"

namespace TANG
{
	class CommandPoolRegistry
	{
	private:

		CommandPoolRegistry();
		CommandPoolRegistry(const CommandPoolRegistry& other) = delete;
		CommandPoolRegistry& operator=(const CommandPoolRegistry& other) = delete;

	public:

		static CommandPoolRegistry& Get()
		{
			static CommandPoolRegistry instance;
			return instance;
		}

		void CreatePools(VkSurfaceKHR surface);
		void DestroyPools();

		VkCommandPool GetCommandPool(QueueType type) const;

	private:

		void CreatePool_Helper(const QueueFamilyIndices& queueFamilyIndices, QueueType type, VkCommandPoolCreateFlags flags);

		std::unordered_map<QueueType, VkCommandPool> pools;

	};

	inline VkCommandPool GetCommandPool(QueueType type)
	{
		return CommandPoolRegistry::Get().GetCommandPool(type);
	}
}

#endif