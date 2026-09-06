#include "NetFrame/Server.hpp"
#include "NetFrame/net/IoWorkerManager.hpp"
#include <memory>

Server::Server() : acceptor(io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 8888)), tickTimer(io)
{
}

Server::~Server()
{
}

void Server::Start()
{
	boost::asio::signal_set signal(io, SIGINT, SIGTERM);

	signal.async_wait([this](const boost::system::error_code& ec, int sigNumber)
		{
			if (ec)
			{
				return;
			}

			Close();

		});


	Tick();
	WaitConnect();
	Log::LogInfo("server start. wait connect...");
	io.run();
}

void Server::WaitConnect()
{
	auto session = IoWorkerManager::Instance()->CreateSession();
	if (!session)
	{
		// 所有 IO worker 已满（达到上限）。继续 accept 后立刻断开该连接，
		// 避免 session 为空时解引用崩溃，同时保证 accept 一直挂起、接受循环不断。
		Log::LogWarning("connect out max limit, drop this connect");
		auto dropSocket = std::make_shared<boost::asio::ip::tcp::socket>(io);
		acceptor.async_accept(*dropSocket, [this, dropSocket](const boost::system::error_code& ec)
			{
				boost::system::error_code closeEc;
				dropSocket->close(closeEc);
				if (acceptor.is_open()) WaitConnect();
			});
		return;
	}

	acceptor.async_accept(session->GetSocket(), [this, session](const boost::system::error_code& ec)
		{
			if (ec)
			{
				Log::LogWarning("client connect failed err_info {" + ec.to_string() + "}");
			}
			else
			{
				IoWorkerManager::Instance()->ConnectSession(session);
				Log::LogInfo("client connect success" + std::to_string(session->id));
			}

			if(acceptor.is_open()) WaitConnect();
		});
}

void Server::Tick()
{
	tickTimer.expires_after(std::chrono::milliseconds(static_cast<long>(Config::FRAME_MS)));

	tickTimer.async_wait([this](const boost::system::error_code& ec)
		{
			if (ec == boost::asio::error::operation_aborted)
			{
				return;
			}
			if (ec)
			{
				Log::LogError("server tick faile " + ec.to_string());
				return;
			}
			Time::frameCount.fetch_add(1);
			Tick();
		});
}

void Server::Close()
{
	io.stop();
	boost::system::error_code ec;
	acceptor.close(ec);
	IoWorkerManager::Instance()->Close();
	Log::LogInfo("server close connect");
}
