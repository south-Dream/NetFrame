#pragma once
#include<iostream>

class Config
{
public:
	// IO_WORKER
	static constexpr std::uint32_t IO_WORKER_INIT_NUM = 1;
	static constexpr std::uint32_t IO_WORKER_MAX_NUM = 3;
	static constexpr std::uint32_t IO_WORKER_MAX_CONNECT = 10;
	// FRAME
	static constexpr std::uint32_t FPS = 20;
	static constexpr std::float_t FRAME = 1.0f / FPS;
	static constexpr std::float_t FRAME_MS = 1000.0f / FPS;
	// Heart
	static constexpr std::uint32_t HEART_CHECK_FRAME_INVERTAL = 100;
	static constexpr std::uint32_t HEART_RECORD_INC = 10;
	static constexpr std::uint32_t PER_HEART_INIT_CAPACITY = 10;
	static constexpr std::uint32_t MAX_MISS_HEART = 3;
};