
#include <fstream>
#include <string.h>
#include <vector>

#include "sanity_check.h"

namespace TANG
{
	// TODO - Replace std::vector return type with a pointer to allocated data, and have an out parameter for
	//        the size of the allocated buffer. This actually sounds kinda nasty, so maybe create a small struct?
	std::vector<char> ReadFile(const std::string& fileName)
	{
		std::ifstream file(fileName, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			LogError("Failed to open file %s!", fileName.c_str());
			return std::vector<char>();
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	void WriteToFile(const std::string& fileName, const std::string& msg)
	{
		std::ofstream file(fileName);
		if (!file.is_open())
		{
			LogError("Failed to write to file '%s'!", fileName.c_str());
		}
		file << msg;

		file.close();
	}

	void AppendToFile(const std::string& fileName, const std::string& msg)
	{
		std::ofstream file(fileName, std::ios::app);
		if (!file.is_open())
		{
			LogError("Failed to append to file '%s'!", fileName.c_str());
		}

		file << msg;

		file.close();
	}

	uint32_t Checksum(const std::string& fileName)
	{
		auto contents = ReadFile(fileName);

		uint32_t checksum = 0;
		uint32_t shift = 0;

		uint32_t numWords = static_cast<uint32_t>(ceil(contents.size() / 4.0f));
		uint32_t* data = reinterpret_cast<uint32_t*>(contents.data());

		for (uint32_t i = 0; i < numWords; i++)
		{
			uint32_t currWord = data[i];
			checksum += (currWord << shift);
			shift = (shift + 8) % 32;
		}

		return checksum;
	}
}