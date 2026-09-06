#pragma once
#include <cmath>
#include <vector>
#include <atomic>
#include <unordered_set>
#include <unordered_map>
#include "NetFrame/session/Session.hpp"
#include "NetFrame/Config.hpp"
#include "NetFrame/tool/Time.hpp"

class SessionManager
{
public:
	SessionManager();
	~SessionManager() = default;

public:
	static constexpr std::uint32_t WheelSize = Config::HEART_CHECK_FRAME_INVERTAL + std::min(static_cast<std::uint32_t>(1), Config::HEART_CHECK_FRAME_INVERTAL);

	std::shared_ptr<Session> Create(const std::uint64_t workerId, boost::asio::io_context& io);
	void Connect(std::shared_ptr<Session> session);
	void Tick();
	std::shared_ptr<Session> Get(const std::uint64_t id) const;

	std::size_t Count() const;

private:
	std::uint64_t GobaleSessionId = 0;
	std::atomic_size_t sessionCount{0};
	std::vector<std::vector<std::uint64_t>> heartRecord;		// Heart
	std::unordered_map<std::uint64_t, std::shared_ptr<Session>> sessionDic;
};
