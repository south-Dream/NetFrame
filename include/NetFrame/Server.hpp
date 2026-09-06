#pragma once
#include <chrono>
#include <atomic>
#include <iostream>
#include <boost/asio.hpp>
#include "NetFrame/Config.hpp"
#include "NetFrame/tool/Log.hpp"
#include "NetFrame/tool/Time.hpp"
#include "NetFrame/session/SessionManager.hpp"

class Server
{
public:
	Server();
	~Server();

	void Start();

	void WaitConnect();

	void Tick();

	void Close();

private:
	std::uint64_t nextTickTimeMs;
	boost::asio::io_context io;
	boost::asio::steady_timer tickTimer;
	boost::asio::ip::tcp::acceptor acceptor;
};
