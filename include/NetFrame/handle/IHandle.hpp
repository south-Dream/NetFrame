#pragma once
#include <iostream>
#include <google/protobuf/message.h>
#include "NetFrame/session/Session.hpp"

class IHandle
{
public:
	IHandle() = default;
	virtual ~IHandle() = default;

	virtual void HandleMessage(Session& session, const google::protobuf::Message& msg) const = 0;
};
