#pragma once

#include "messages/message_types.h"
#include "serialization/serializable.h"

namespace AqualinkAutomate::Messages
{

	class Message : public AqualinkAutomate::Serialization::Serializable
	{
	protected:
		Message(MessageTypes type) :
			m_Type(type)
		{
		}

	private:
		const MessageTypes m_Type;
	};

}
// namespace AqualinkAutomate::Messages
