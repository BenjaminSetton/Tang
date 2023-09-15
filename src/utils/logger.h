#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>

// Define simple logging functions
namespace TANG
{
	static inline void LogError(std::string_view msg)
	{
		std::cerr << "[ERROR] " << msg << std::endl;
	}

	static inline void LogWarning(std::string_view msg)
	{
		std::cerr << "[WARNING] " << msg << std::endl;
	}

	static inline void LogInfo(std::string_view msg)
	{
		std::cerr << "[INFO] " << msg << std::endl;
	}
}

#endif