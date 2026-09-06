#pragma once
#include <iostream>
#include <unordered_map>
#include "NetFrame/handle/IHandle.hpp"
#include "NetFrame/message/MessageManager.hpp"

class HandleManager;
template<typename TMessage, typename THandle>
class MHRegister;

#define REGISTER_M_H(MESSAGE_ID, MESSAGE_CLASS, HANDLE_CLASS)\
	inline MHRegister<MESSAGE_CLASS, HANDLE_CLASS> g_##MESSAGE_CLASS_##HANDLE_CLASS_register(MESSAGE_ID);

template<typename TMessage, typename THandle>
class MHRegister
{
public:
	MHRegister(const std::uint32_t id)
	{
		MessageManager::Instance()->Register<TMessage>(id);
		HandleManager::Instance()->Register<THandle, TMessage>();
	}
};



class HandleManager
{
private:
	HandleManager() = default;
	~HandleManager() = default;
	static HandleManager* instance;

public:
	static HandleManager* Instance();

	template<typename THandle, typename TMessage>
	void Register();

	void HandleMessage(Session& session, const google::protobuf::Message& msg) const;

private:
	// 每种消息只保存一个无状态 Handle。Handle 会被多个 IoWorker 线程共享调用，
	// 连接状态应保存在 Session 中，共享业务状态由对应业务模块负责同步。
	std::unordered_map<std::type_index, std::unique_ptr<IHandle>> handleDic;
};

template<typename THandle, typename TMessage>
void HandleManager::Register()
{
	std::type_index ti = typeid(TMessage);
	auto it = handleDic.find(ti);
	if (it != handleDic.end())
	{
		return;
	}
	handleDic[ti] = std::make_unique<THandle>();
}
