#pragma once

#include "messages/message.h"
#include "messages/message_types.h"

namespace AqualinkAutomate::Messages
{

	class UnknownMessage : public Message
	{
		static constexpr MessageTypes MESSAGE_TYPE = { MessageTypes::Unknown };

	public:
		UnknownMessage() : Message(MESSAGE_TYPE) {};

	public:
		void serialize(std::ostream& stream) const override;
		void deserialize(std::istream& stream) override;
	};

}
// namespace AqualinkAutomate::Messages
