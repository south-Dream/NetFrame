#pragma once
#include<iostream>
#include<chrono>
#include<cstdint>
#include<atomic>

class Server;

class Time
{
	friend Server;
private:
	/// <summary>
	/// 游戏运行帧数
	/// </summary>
	static std::atomic_uint64_t frameCount;

public:
	/// <summary>
	/// 游戏运行帧数
	/// </summary>
	/// <returns></returns>
	static std::uint64_t FrameCount();

public:
	/// <summary>
	/// 获取系统毫秒时间戳
	/// </summary>
	/// <returns></returns>
	static std::uint64_t GetTimeStampMs();

	/// <summary>
	/// 获取游戏运行至今的毫秒时间戳
	/// </summary>
	/// <returns></returns>
	static std::uint64_t GetSteadyMs();
};