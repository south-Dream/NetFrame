#pragma once
#include <iostream>
#include <typeindex>
#include <memory>
#include <unordered_map>
#include <functional>
#include <google/protobuf/message.h>

class MessageManager;
template<typename TMessage>
class MessageRegister;

#define REGISTER_M(MESSAGE_ID, MESSAGE_CLASS)\
	inline MessageRegister<MESSAGE_CLASS> g_##MESSAGE_CLASS_register(MESSAGE_ID);


template<typename TMessage>
class MessageRegister
{
public:
	MessageRegister(const std::uint32_t id)
	{
		MessageManager::Instance()->Register<TMessage>(id);
	}
	~MessageRegister() = default;
};

class MessageManager
{
private:
	MessageManager() = default;
	~MessageManager() = default;
	static MessageManager* instance;

public:
	using MessagePtr = std::unique_ptr<google::protobuf::Message>;
	using MessageCtor = std::function<MessagePtr()>;

	static MessageManager* Instance();

	std::uint32_t GetMessageId(const std::type_index ti) const;

	template<typename TMessage>
	void Register(const std::uint32_t id);

	MessagePtr Create(const std::uint32_t id) const;

private:
	std::unordered_map<std::uint32_t, MessageCtor> messageCtorMapper;
	std::unordered_map<std::type_index, std::uint32_t> messageIdMaper;
};


template<typename TMessage>
void MessageManager::Register(const std::uint32_t id)
{
	auto it = messageCtorMapper.find(id);
	if (it != messageCtorMapper.end())
	{
		return;
	}

	messageCtorMapper[id] = []()
		{
			return std::make_unique<TMessage>();
		};

	messageIdMaper[typeid(TMessage)] = id;
}