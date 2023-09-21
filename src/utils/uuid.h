#ifndef UUID_H
#define UUID_H

#include <random>
#include <iostream>

namespace TANG
{
	typedef uint64_t UUID;

	UUID GetUUID();
}

#endif