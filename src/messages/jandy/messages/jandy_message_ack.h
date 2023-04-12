#pragma once

#include "messages/jandy/messages/jandy_message.h"

namespace AqualinkAutomate::Messages::Jandy::Messages
{

	class JandyAckMessage : public JandyMessage
	{
	public:
		JandyAckMessage();
		virtual ~JandyAckMessage();

	public:
		void Deserialize(const std::span<const std::byte>& message_bytes);
	};

}
// namespace AqualinkAutomate::Messages::Jandy::Messages
