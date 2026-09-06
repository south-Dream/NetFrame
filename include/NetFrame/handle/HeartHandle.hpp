#pragma once
#include "NetFrame/handle/Handle.hpp"
#include "NetFrame/handle/HandleManager.hpp"
#include "proto/generated/ping.pb.h"
#include "proto/generated/pong.pb.h"

class HeartHandle : public Handle<Ping>
{
protected:
	void HandleMessage(Session& session, const Ping& msg) const override;
};

REGISTER_M(1002, Pong);
REGISTER_M_H(1001, Ping, HeartHandle);
