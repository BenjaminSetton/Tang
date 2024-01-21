
#include <iostream>
#include <cstdarg>
#include <time.h>

#include "logger.h"

#define LOG_FORMAT_CODE(logType) \
	char buffer[MAX_BUFFER_SIZE_BYTES]; \
	va_list va; \
	va_start(va, format); \
	vsprintf_s(buffer, format, va); \
	Log_Internal(logType, buffer); \
	va_end(va);

static constexpr uint32_t MAX_BUFFER_SIZE_BYTES = 250;

// ANSI color codes
static const char* LogErrorColor = "\x1B[31m";
static const char* LogWarningColor = "\x1B[33m";
static const char* LogInfoColor = "\x1B[37m";
static const char* LogDebugColor = "\x1B[36m";
static const char* LogClearColor = "\033[0m";

// Log type buffers
static const char* LogErrorBuffer = "ERROR";
static const char* LogWarningBuffer = "WARNING";
static const char* LogInfoBuffer = "INFO";
static const char* LogDebugBuffer = "DEBUG";

// Define simple logging functions
namespace TANG
{
	void Log_Internal(LogType type, const char* message)
	{
		const char* logTypeColor = nullptr;
		const char* logTypeBuffer = nullptr;

		switch (type)
		{
		case LogType::DEBUG:
		{
			logTypeColor = LogDebugColor;
			logTypeBuffer = LogDebugBuffer;
			break;
		}
		case LogType::INFO:
		{
			logTypeColor = LogInfoColor;
			logTypeBuffer = LogInfoBuffer;
			break;
		}
		case LogType::WARNING:
		{
			logTypeColor = LogWarningColor;
			logTypeBuffer = LogWarningBuffer;
			break;
		}
		case LogType::ERR:
		{
			logTypeColor = LogErrorColor;
			logTypeBuffer = LogErrorBuffer;
			break;
		}
		}

		// Get the timestamp
		std::time_t currentTime = std::time(nullptr);
		std::tm currentLocalTime{};
		localtime_s(&currentLocalTime, &currentTime);

		int hour = currentLocalTime.tm_hour;
		char AMorPM[3] = "AM"; // Contains "AM" or "PM" including null terminator

		if (hour > 12)
		{
			hour -= 12;
			memcpy(AMorPM, "PM", 3);
		}

		fprintf_s(stderr, "[%02d:%02d:%02d %s] %s[%s] %s %s\n", 
			hour,
			currentLocalTime.tm_min,
			currentLocalTime.tm_sec,
			AMorPM,
			logTypeColor,
			logTypeBuffer,
			message,
			LogClearColor);
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