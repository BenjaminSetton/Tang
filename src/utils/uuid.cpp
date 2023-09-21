
#include "uuid.h"

namespace TANG
{
	// Adapted from https://stackoverflow.com/questions/21096015/how-to-generate-64-bit-random-numbers
	UUID GetUUID()
	{
		// TODO - Make this a lot better. I just need something to work
		std::random_device device;
		std::mt19937_64 engine(device());
		std::uniform_int_distribution<long long int> dist(std::llround(std::pow(2, 61)), std::llround(std::pow(2, 62)));

		return dist(engine);
	}
}