#pragma once
#include <string>

class Log
{
public:
	enum LogLevel
	{
		Info = 1 << 0,
		Warning = 1 << 1,
		Error = 1 << 2,
	};
public:
	static void LogInfo(const std::string& msg);
	static void LogError(const std::string& msg);
	static void LogWarning(const std::string& msg);

public:
	static bool timeEnable;
	static bool colorEnable;
	static std::uint32_t logLevel;
};
