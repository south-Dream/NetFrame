#pragma once
#include <iostream>
#include <boost/asio.hpp>
#include <vector>
#include <array>
#include <queue>
#include <memory>
#include <google/protobuf/message.h>
#include "NetFrame/message/MessageManager.hpp"
#include "proto/generated/ping.pb.h"
#include "proto/generated/pong.pb.h"

class Session : public std::enable_shared_from_this<Session>
{
public:
	Session(const std::uint64_t id, boost::asio::io_context& io);
	~Session();

	void Start();
	void Close();

	boost::asio::ip::tcp::socket& GetSocket();

private:
	void RecvHead();
	void RecvBody(const std::uint32_t msgId);
	void HandleRecvMessage(const google::protobuf::Message& msg);
	void HandleSendMessage();

	std::uint32_t ReadUInt32(const char* data);
	void WriteUInt32(char* data, std::uint32_t value);

public:
	template<typename TMessage>
	void Send(TMessage msg);

	void SetHeart();
	std::uint32_t GetHeart();
	std::uint64_t GetLastSteadyChrono();
	bool MissHeart();

public:
	inline static constexpr std::size_t MAX_MESSAGE_LENGTH = 1024 * 1024;
	const std::uint64_t id;

private:
	bool isClose = false;
	bool isSending = false;

	boost::asio::any_io_executor excutor;
	boost::asio::ip::tcp::socket socket;
	std::array<char, 8> recvHeadBuffer{};
	std::vector<char> recvBodyBuffer;
	std::queue<std::vector<char>> sendQueue;

	// Heart
	std::uint32_t missHeartCnt;
	std::uint32_t checkHeartFrame;
	std::uint64_t lastSteayChrono;
};

template<typename TMessage>
void Session::Send(TMessage msg)
{
	boost::asio::post(excutor,
		[self = shared_from_this(), msg = std::move(msg)]()
		{
			if (self->isClose) return;
			uint32_t bodyLen = static_cast<uint32_t>(msg.ByteSizeLong());

			if (bodyLen > Session::MAX_MESSAGE_LENGTH) return;

			std::vector<char> msgBytes(bodyLen + 8);
			self->WriteUInt32(msgBytes.data(), MessageManager::Instance()->GetMessageId(typeid(msg)));
			self->WriteUInt32(msgBytes.data() + 4, bodyLen);

			if (!msg.SerializeToArray(msgBytes.data() + 8, static_cast<int>(bodyLen))) return;
			self->sendQueue.push(std::move(msgBytes));

			if (!self->isSending)
				self->HandleSendMessage();
		});
}
