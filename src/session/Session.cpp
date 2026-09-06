#include "NetFrame/session/Session.hpp"
#include "NetFrame/session/SessionManager.hpp"
#include "NetFrame/handle/HandleManager.hpp"
#include "NetFrame/message/MessageManager.hpp"
#include "proto/generated/ping.pb.h"
#include "proto/generated/pong.pb.h"
#include "NetFrame/net/IoWorkerManager.hpp"

Session::Session(const std::uint64_t id, boost::asio::io_context& io)
	: id(id), socket(io), excutor(io.get_executor())
{
}
Session::~Session()
{
}

void Session::Start()
{
	RecvHead();
}
void Session::Close()
{
	if (!isClose)
	{
		isClose = true;
		boost::system::error_code ec;
		socket.close(ec);
		checkHeartFrame = -1;
	}
}

boost::asio::ip::tcp::socket& Session::GetSocket()
{
	return socket;
}


void Session::RecvHead()
{
	if (isClose) return;
	auto self = shared_from_this();
	boost::asio::async_read(socket, boost::asio::buffer(recvHeadBuffer), [self](const boost::system::error_code& ec, std::size_t byteLen)
		{
			if (ec)
			{
				self->Close();
				return;
			}
			std::uint32_t msgId = self->ReadUInt32(self->recvHeadBuffer.data());
			std::uint32_t bodylen = self->ReadUInt32(self->recvHeadBuffer.data() + 4);
			if (bodylen > Session::MAX_MESSAGE_LENGTH)
			{
				self->Close();
				return;
			}

			self->recvBodyBuffer.resize(bodylen);
			self->RecvBody(msgId);
		});
}
void Session::RecvBody(const uint32_t msgId)
{
	if (isClose) return;
	auto self = shared_from_this();
	boost::asio::async_read(socket, boost::asio::buffer(recvBodyBuffer), [self, msgId](const boost::system::error_code& ec, std::size_t byteLen)
		{
			if (ec)
			{
				self->Close();
				return;
			}

			MessageManager::MessagePtr msg = MessageManager::Instance()->Create(msgId);
			if (!msg || !msg->ParseFromArray(self->recvBodyBuffer.data(), static_cast<int>(self->recvBodyBuffer.size())))
			{
				self->Close();
				return;
			}

			self->HandleRecvMessage(*msg);
			self->RecvHead();
		});
}
void Session::HandleRecvMessage(const google::protobuf::Message& msg)
{
	if (isClose) return;
	HandleManager::Instance()->HandleMessage(*this, msg);
}

void Session::HandleSendMessage()
{

	if (isClose) return;
	if (sendQueue.empty())
	{
		isSending = false;
		return;
	}

	isSending = true;
	auto self = shared_from_this();
	boost::asio::async_write(socket, boost::asio::buffer(sendQueue.front()), [self](const boost::system::error_code& ec, std::size_t size)
		{
			if (ec)
			{
				self->Close();
				return;
			}

			self->sendQueue.pop();
			self->HandleSendMessage();
		});

}

std::uint32_t Session::ReadUInt32(const char* data)
{
	return (static_cast<std::uint32_t>(
		static_cast<unsigned char>(data[0])) << 24) |
		(static_cast<std::uint32_t>(
			static_cast<unsigned char>(data[1])) << 16) |
		(static_cast<std::uint32_t>(
			static_cast<unsigned char>(data[2])) << 8) |
		static_cast<std::uint32_t>(
			static_cast<unsigned char>(data[3]));
}

void Session::WriteUInt32(char* data, std::uint32_t value)
{
	data[0] = static_cast<char>((value >> 24) & 0xff);
	data[1] = static_cast<char>((value >> 16) & 0xff);
	data[2] = static_cast<char>((value >> 8) & 0xff);
	data[3] = static_cast<char>(value & 0xff);
}

void Session::SetHeart()
{
	missHeartCnt = 0;
	checkHeartFrame = (Time::FrameCount() + Config::HEART_CHECK_FRAME_INVERTAL) % SessionManager::WheelSize;
	lastSteayChrono = Time::GetSteadyMs();
}
std::uint32_t Session::GetHeart()
{
	return checkHeartFrame;
}
std::uint64_t Session::GetLastSteadyChrono()
{
	return lastSteayChrono;
}
bool Session::MissHeart()
{
	return ++missHeartCnt >= Config::MAX_MISS_HEART;
}
