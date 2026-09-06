#include "NetFrame/handle/HandleManager.hpp"
#include "NetFrame/session/Session.hpp"

HandleManager* HandleManager::instance = nullptr;

HandleManager* HandleManager::Instance()
{
	if (instance == nullptr)
		instance = new HandleManager();
	return instance;
}

void HandleManager::HandleMessage(Session& session, const google::protobuf::Message& msg) const
{
	auto it = handleDic.find(typeid(msg));
	if (it == handleDic.end())
	{
		return;
	}
	it->second->HandleMessage(session, msg);
}
