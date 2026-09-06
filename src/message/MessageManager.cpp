#include "NetFrame/message/MessageManager.hpp"

MessageManager* MessageManager::instance = nullptr;

MessageManager* MessageManager::Instance()
{
	if (instance == nullptr)
		instance = new MessageManager();
	return instance;
}

std::uint32_t MessageManager::GetMessageId(const std::type_index ti) const
{
	auto it = messageIdMaper.find(ti);
	if (it == messageIdMaper.end()) return 0;
	return it->second;
}

MessageManager::MessagePtr MessageManager::Create(const std::uint32_t id) const
{
	auto it = messageCtorMapper.find(id);
	if (it == messageCtorMapper.end()) return nullptr;
	return it->second();
}
