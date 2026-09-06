#include "NetFrame/net/IoWorker.hpp"
#include "NetFrame/tool/Log.hpp"


IoWorker::IoWorker(const std::uint8_t id) : id(id), tickTimer(io)
{
}

IoWorker::~IoWorker()
{
	Close();
}

void IoWorker::Start()
{
	Tick();
	thread = std::thread([this]()
		{
			io.run();
		});
}
void IoWorker::Tick()
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
				Log::LogError("io tick faile " + ec.to_string());
				return;
			}

			// 更新Session
			sessionManager.Tick();

			Tick();
		});
}
void IoWorker::Close()
{
	tickTimer.cancel();
	io.stop();

	if (thread.joinable() && thread.get_id() != std::this_thread::get_id())
	{
		thread.join();
	}
}

std::shared_ptr<Session> IoWorker::CreateSession()
{
	// 只创建，所以直接返回
	return sessionManager.Create(static_cast<std::uint64_t>(id) << 56, io);
}
void IoWorker::ConnectSession(std::shared_ptr<Session> session)
{
	boost::asio::post(io, [this, session]()
		{
			sessionManager.Connect(session);
			session->Start();
		});
}
void IoWorker::GetSession(const std::uint64_t id, std::function<void(std::shared_ptr<Session>)> callback)
{
	boost::asio::post(io, [this, id, callback]()
		{
			callback(sessionManager.Get(id));
		});
}

std::size_t IoWorker::SessionCount() const
{
	return sessionManager.Count();
}
bool IoWorker::IsFull() const
{
	return SessionCount() >= static_cast<std::size_t>(Config::IO_WORKER_MAX_CONNECT);
}
