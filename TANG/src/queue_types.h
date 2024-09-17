#ifndef QUEUE_TYPES_H
#define QUEUE_TYPES_H

// Might wanna reconsider removing this.
// This file can be included in a ton of places
#include <vulkan/vulkan.h>
//

namespace TANG
{
	struct QueueSubmitInfo
	{
		VkSemaphore waitSemaphore;
		VkSemaphore signalSemaphore;
		VkFence fence;
		VkPipelineStageFlags waitStages;
	};

	enum class QUEUE_TYPE
	{
		GRAPHICS,
		PRESENT,
		TRANSFER,
		COMPUTE,
		COUNT			// NOTE - This value must come last
	};
}

#endif