#pragma once
#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include "NetFrame/session/SessionManager.hpp"
#include "NetFrame/Config.hpp"

class IoWorker
{
public:
	IoWorker(const std::uint8_t id);
	~IoWorker();

	void Start();
	void Close();

	std::shared_ptr<Session> CreateSession();
	void ConnectSession(std::shared_ptr<Session>);
	void GetSession(const std::uint64_t id, std::function<void(std::shared_ptr<Session>)> callback);

	std::size_t SessionCount() const;
	bool IsFull() const;

private:
	void Tick();
private:
	const std::uint8_t id;
	std::thread thread;
	boost::asio::io_context io;
	boost::asio::steady_timer tickTimer;
	SessionManager sessionManager;
};
