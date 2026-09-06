#include "NetFrame/tool/Log.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace
{
	constexpr const char* Yellow = "\033[33m";
	constexpr const char* Red = "\033[31m";
	constexpr const char* Reset = "\033[0m";

	std::mutex outputMutex;

	bool IsMatched(const Log::LogLevel level)
	{
		return (static_cast<int>(Log::logLevel) & static_cast<int>(level)) != 0;
	}

	std::string CurrentTime()
	{
		const auto now = std::chrono::system_clock::now();
		const std::time_t value = std::chrono::system_clock::to_time_t(now);
		std::tm localTime{};

#ifdef _WIN32
		localtime_s(&localTime, &value);
#else
		localtime_r(&value, &localTime);
#endif

		std::ostringstream stream;
		stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
		return stream.str();
	}

	void WriteLog(const Log::LogLevel level, const char* levelName,
		const char* color, const std::string& msg)
	{
		if (!IsMatched(level))
		{
			return;
		}

		std::lock_guard<std::mutex> lock(outputMutex);

		if (Log::colorEnable && color != nullptr)
		{
			std::cout << color;
		}

		std::cout << '[' << levelName;
		if (Log::timeEnable)
		{
			std::cout << ' ' << CurrentTime();
		}
		std::cout << "] " << msg;

		if (Log::colorEnable && color != nullptr)
		{
			std::cout << Reset;
		}

		std::cout << std::endl;
	}
}

bool Log::timeEnable = false;
bool Log::colorEnable = false;
std::uint32_t Log::logLevel = Log::Info;

void Log::LogInfo(const std::string& msg)
{
	WriteLog(Info, "info", nullptr, msg);
}

void Log::LogWarning(const std::string& msg)
{
	WriteLog(Warning, "warning", Yellow, msg);
}

void Log::LogError(const std::string& msg)
{
	WriteLog(Error, "error", Red, msg);
}
