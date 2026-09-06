#include "NetFrame/handle/HeartHandle.hpp"

void HeartHandle::HandleMessage(Session& session, const Ping& msg) const
{
	Pong pong;
	pong.set_sequence(msg.sequence());
	pong.set_clientsendtime(msg.clientsendtime());
	pong.set_serversendtime(1);
	session.SetHeart();
	session.Send<Pong>(pong);
}
