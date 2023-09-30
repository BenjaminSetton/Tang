#ifndef UUID_H
#define UUID_H

#include <iostream>
#include <limits>
#include <random>

namespace TANG
{
	typedef uint64_t UUID;

	static constexpr UUID INVALID_UUID = std::numeric_limits<UUID>::max();

	UUID GetUUID();
}

#endif