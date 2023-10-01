
#include "logger.h"

#include <cstdarg>
#include <iostream>

#define LOG_FORMAT_CODE(logType) \
	char buffer[MAX_BUFFER_SIZE_BYTES]; \
	va_list va; \
	va_start(va, format); \
	vsprintf_s(buffer, format, va); \
	Log_Internal(logType, buffer); \
	va_end(va);

static constexpr uint32_t MAX_BUFFER_SIZE_BYTES = 250;

// Define simple logging functions
namespace TANG
{
	void Log_Internal(LogType type, const char* buffer)
	{
		switch (type)
		{
		case LogType::DEBUG:
		{
			std::cerr << "[DEBUG] " << buffer << std::endl;
			break;
		}
		case LogType::INFO:
		{
			std::cerr << "[INFO] " << buffer << std::endl;
			break;
		}
		case LogType::WARNING:
		{
			std::cerr << "[WARNING] " << buffer << std::endl;
			break;
		}
		case LogType::ERR:
		{
			std::cerr << "[ERROR] " << buffer << std::endl;
			break;
		}
		}
	}

	void LogError(const char* format, ...)
	{
		LOG_FORMAT_CODE(LogType::ERR);
	}

	void LogWarning(const char* format, ...)
	{
		LOG_FORMAT_CODE(LogType::WARNING);
	}

	void LogInfo(const char* format, ...)
	{
		LOG_FORMAT_CODE(LogType::INFO);
	}
}