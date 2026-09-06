#pragma once
#include <iostream>
#include <cmath>
#include "NetFrame/net/IoWorker.hpp"
#include "NetFrame/Config.hpp"

class IoWorkerManager
{
public:
	IoWorkerManager() = default;
	~IoWorkerManager() = default;
	static IoWorkerManager* Instance();

	void Start();
	void Close();
	std::shared_ptr<IoWorker> GetIoWorker(const std::uint8_t workerId) const;
	std::shared_ptr<IoWorker> GetIoWorker(const std::uint64_t sessionId) const;

	std::shared_ptr<Session> CreateSession();
	void ConnectSession(std::shared_ptr<Session>);
	void GetSession(const std::uint64_t id, std::function<void(std::shared_ptr<Session>)> callback) const;


private:
	static IoWorkerManager* instance;

	std::shared_ptr<IoWorker> CreateIoWorker();

private:
	static std::uint8_t GobalWorkerId;
	std::unordered_map<std::uint8_t, std::shared_ptr<IoWorker>> ioWorkerDic;
};
