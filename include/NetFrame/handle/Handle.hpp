#pragma once
#include "NetFrame/handle/IHandle.hpp"


template<typename TMessage>
class Handle : public IHandle
{
public:
	Handle() = default;
	~Handle() override = default;

	void HandleMessage(Session& session, const google::protobuf::Message& msg) const override
	{
		HandleMessage(session, static_cast<const TMessage&>(msg));
	}

protected:
	virtual void HandleMessage(Session& session, const TMessage& msg) const = 0;
};
