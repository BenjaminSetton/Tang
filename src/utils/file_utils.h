
#include <string.h>

namespace TANG
{
	// TODO - Replace std::vector return type with a pointer to allocated data, and have an out parameter for
	//        the size of the allocated buffer. This actually sounds kinda nasty, so maybe create a small struct?
	std::vector<char> ReadFile(const std::string& fileName);

	void WriteToFile(const std::string& fileName, const std::string& msg);

	void AppendToFile(const std::string& fileName, const std::string& msg);

	uint32_t FileChecksum(const std::string& fileName);
}