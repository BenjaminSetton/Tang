#ifndef LOGGER_H
#define LOGGER_H

enum class LogType
{
	DEBUG = -1,
	INFO,
	WARNING,
	ERR,
};

// Define simple logging functions
namespace TANG
{
	void Log_Internal(LogType type, const char* buffer);

	void LogError(const char* format, ...);

	void LogWarning(const char* format, ...);

	void LogInfo(const char* format, ...);
}

#endif