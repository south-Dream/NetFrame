#include "NetFrame/net/IoWorkerManager.hpp"
#include "NetFrame/tool/Log.hpp"
#include <functional>

IoWorkerManager* IoWorkerManager::instance = nullptr;
std::uint8_t IoWorkerManager::GobalWorkerId = 0;

IoWorkerManager* IoWorkerManager::Instance()
{
	if (instance == nullptr)
		instance = new IoWorkerManager();
	return instance;
}

void IoWorkerManager::Start()
{
	std::size_t num = std::min(static_cast<std::size_t>(
		Config::IO_WORKER_INIT_NUM), static_cast<std::size_t>(1));
	for (int i = 0; i < num; i++) CreateIoWorker();
}
void IoWorkerManager::Close()
{
	for (auto& kvp : ioWorkerDic) kvp.second->Close();
}


std::shared_ptr<IoWorker> IoWorkerManager::CreateIoWorker()
{
	std::shared_ptr<IoWorker> worker = std::make_shared<IoWorker>(GobalWorkerId);
	ioWorkerDic[GobalWorkerId++] = worker;
	worker->Start();
	return worker;
}

std::shared_ptr<IoWorker> IoWorkerManager::GetIoWorker(const std::uint8_t workerId) const
{
	auto it = ioWorkerDic.find(workerId);
	if (it == ioWorkerDic.end()) return nullptr;
	return it->second;
}
std::shared_ptr<IoWorker> IoWorkerManager::GetIoWorker(const std::uint64_t sessionId) const
{
	uint8_t workerId = sessionId >> 56;
	return GetIoWorker(workerId);
}



std::shared_ptr<Session> IoWorkerManager::CreateSession()
{
	for (auto& [id, worker] : ioWorkerDic)
	{
		if (!worker->IsFull())
		{
			return worker->CreateSession();
		}
	}
	if (ioWorkerDic.size() >= Config::IO_WORKER_MAX_NUM)
	{
		Log::LogWarning("connect out max limit");
		return nullptr;
	}
	std::shared_ptr<IoWorker> worker = CreateIoWorker();
	return worker->CreateSession();
}
void IoWorkerManager::ConnectSession(std::shared_ptr<Session> session)
{
	auto worker = GetIoWorker(session->id);
	worker->ConnectSession(session);
}
void IoWorkerManager::GetSession(const std::uint64_t id, std::function<void(std::shared_ptr<Session>)> callback) const
{
	std::uint8_t workerId = static_cast<std::uint8_t>(id >> 56);
	auto it = ioWorkerDic.find(workerId);
	if (it == ioWorkerDic.end())
	{
		callback(nullptr);
		return;
	}
	it->second->GetSession(id, callback);
}
