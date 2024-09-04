#ifndef DISPOSABLE_COMMAND_H
#define DISPOSABLE_COMMAND_H

#include "vulkan/vulkan.h"

#include "../command_pool_registry.h"
#include "../queue_types.h"

namespace TANG
{
	// Thin wrapper around RAII disposable (single-use) command buffer, usually used to transfer images from one
	// format to another, or similar trivial tasks. Copying this object is not supported.
	// This is a special command buffer, and therefore doesn't inherit from the Buffer class
	class DisposableCommand
	{
	public:

		explicit DisposableCommand(QueueType type, bool waitUntilQueueIdle);
		~DisposableCommand();
		DisposableCommand(const DisposableCommand& other) = delete;
		DisposableCommand(DisposableCommand&& other) noexcept = delete;
		DisposableCommand& operator=(const DisposableCommand& other) = delete;

		VkCommandBuffer GetBuffer() const;

	private:

		VkDevice logicalDeviceHandle;  // It's probably safe to copy this handle because the lifetime of this object is intended to be super short
		QueueType type;
		VkCommandBuffer allocatedBuffer;
		bool waitUntilQueueIdle;
	};
}

#endif