#ifndef QUEUE_TYPES_H
#define QUEUE_TYPES_H

namespace TANG
{
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