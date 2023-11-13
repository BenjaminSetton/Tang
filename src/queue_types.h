#ifndef QUEUE_TYPES_H
#define QUEUE_TYPES_H

namespace TANG
{
	enum class QueueType
	{
		GRAPHICS,
		PRESENT,
		TRANSFER,
		COUNT			// NOTE - This value must come last
	};
}

#endif