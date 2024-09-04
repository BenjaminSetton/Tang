
#include <fstream>
#include <vector>

#include "sanity_check.h"
#include "logger.h"

namespace TANG
{
	uint32_t ReadFile(const std::string_view& fileName, char** outBuffer)
	{
		if (outBuffer && (*outBuffer != nullptr))
		{
			LogError("Failed to read file, out-buffer must not already contain any data!");
			return 0;
		}

		std::ifstream file(fileName.data(), std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			LogError("Failed to open file %s!", fileName.data());
			return 0;
		}

		uint32_t fileSize = static_cast<uint32_t>(file.tellg());
		*outBuffer = new char[fileSize];

		file.seekg(0);
		file.read(*outBuffer, fileSize);

		file.close();

		return fileSize;
	}

	uint32_t ReadFile(const std::string_view& fileName, char* outBuffer, uint32_t maxBufferSize, bool allowIncompleteRead)
	{
		if (outBuffer == nullptr)
		{
			LogError("Failed to read file, out-buffer is null!");
			return 0;
		}

		std::ifstream file(fileName.data(), std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			LogError("Failed to open file '%s'!", fileName.data());
			return 0;
		}

		uint32_t fileSize = static_cast<uint32_t>(file.tellg());
		if (fileSize >= maxBufferSize && !allowIncompleteRead)
		{
			LogWarning("Failed to read contents of file '%s', max buffer size (%u) is less than or equal to file size (%u) and incomplete reads are disallowed!", fileName.data(), maxBufferSize, static_cast<uint32_t>(fileSize));
			return 0;
		}

		fileSize = std::min(fileSize, maxBufferSize);

		file.seekg(0);
		file.read(outBuffer, fileSize);

		file.close();

		return fileSize;
	}

	void WriteToFile(const std::string_view& fileName, const std::string_view& msg)
	{
		std::ofstream file(fileName.data());
		if (!file.is_open())
		{
			LogError("Failed to write to file '%s'!", fileName.data());
		}
		file << msg;

		file.close();
	}

	void AppendToFile(const std::string_view& fileName, const std::string_view& msg)
	{
		std::ofstream file(fileName.data(), std::ios::app);
		if (!file.is_open())
		{
			LogError("Failed to append to file '%s'!", fileName.data());
		}

		file << msg;

		file.close();
	}

	uint32_t FileChecksum(const std::string_view& fileName)
	{
		char* contents = nullptr;
		uint32_t bytesRead = ReadFile(fileName, &contents);
		if (bytesRead == 0)
		{
			// Failed to read file, return invalid checksum
			return 0;
		}

		uint32_t checksum = 0;
		uint32_t shift = 0;

		uint32_t numWords = static_cast<uint32_t>(ceil(bytesRead / 4.0f));
		uint32_t* data = reinterpret_cast<uint32_t*>(contents);

		for (uint32_t i = 0; i < numWords; i++)
		{
			uint32_t currWord = data[i];
			checksum += (currWord << shift);
			shift = (shift + 8) % 32;
		}

		delete[] contents;

		return checksum;
	}
}