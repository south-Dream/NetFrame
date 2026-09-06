#include "NetFrame/tool/Time.hpp"

/// <summary>
/// 游戏运行帧数
/// </summary>
std::atomic_uint64_t Time::frameCount = 0;

/// <summary>
/// 游戏运行帧数
/// </summary>
/// <returns></returns>
std::uint64_t Time::FrameCount()
{
	return frameCount.load();
}

/// <summary>
/// 获取系统毫秒时间戳
/// </summary>
/// <returns></returns>
std::uint64_t Time::GetTimeStampMs()
{
	auto now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();
	return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}

/// <summary>
/// 获取游戏运行至今的毫秒时间戳
/// </summary>
/// <returns></returns>
std::uint64_t Time::GetSteadyMs()
{
	auto now = std::chrono::steady_clock::now();
	auto duration = now.time_since_epoch();
	return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}