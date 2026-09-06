#include "NetFrame/session/SessionManager.hpp"

SessionManager::SessionManager()
{
	// 必须确保心跳桶容量 > 心跳间隔
	heartRecord.resize(WheelSize);
	for (int i = 0; i < heartRecord.size(); i++)
	{
		heartRecord[i].reserve(Config::PER_HEART_INIT_CAPACITY);
	}
}

std::shared_ptr<Session> SessionManager::Create(const std::uint64_t workerId, boost::asio::io_context& io)
{
	std::uint64_t localId = GobaleSessionId++;
	std::uint64_t sessionId = workerId + localId;
	std::shared_ptr<Session> session = std::make_shared<Session>(sessionId, io);
	return session;
}

void SessionManager::Connect(std::shared_ptr<Session> session)
{
	auto it = sessionDic.find(session->id);
	if (it != sessionDic.end()) return;
	session->SetHeart();
	heartRecord[session->GetHeart()].push_back(session->id);
	sessionDic[session->id] = session;
	sessionCount.fetch_add(1);
}

void SessionManager::Tick()
{
	const std::size_t idx = Time::FrameCount() % WheelSize;
	const std::size_t nextIdx = (idx + Config::HEART_CHECK_FRAME_INVERTAL) % WheelSize;
	// 一个检测周期的毫秒数：帧间隔 * 周期帧数
	const std::uint64_t checkIntervalMs =
		static_cast<std::uint64_t>(Config::FRAME_MS) * Config::HEART_CHECK_FRAME_INVERTAL;
	const std::uint64_t nowMs = Time::GetSteadyMs();

	for (std::size_t i = 0; i < heartRecord[idx].size(); i++)
	{
		const std::uint64_t id = heartRecord[idx][i];
		auto it = sessionDic.find(id);
		if (it == sessionDic.end()) continue;
		if (it->second->GetHeart() == -1)		// -1为Session主动断开
		{
			sessionDic.erase(it);
			sessionCount.fetch_sub(1);
			continue;
		}
		// 距上次心跳已超过一个检测周期 => 记一次漏跳；
		// 连续漏跳达到 MAX_MISS_HEART 次则强制断开（会话被移入桶后约每间隔被访问一次）
		if (nowMs >= it->second->GetLastSteadyChrono() + checkIntervalMs && it->second->MissHeart())
		{
			it->second->Close();				// 这里心跳检测超时，强制断开
			sessionCount.fetch_sub(1);
			sessionDic.erase(it);
			continue;
		}
		heartRecord[nextIdx].push_back(id);
	}
	heartRecord[idx].clear();
}

std::shared_ptr<Session> SessionManager::Get(const std::uint64_t id) const
{
	auto it = sessionDic.find(id);
	if (it == sessionDic.end()) return nullptr;
	return it->second;
}

std::size_t SessionManager::Count() const
{
	return sessionCount.load();
}
