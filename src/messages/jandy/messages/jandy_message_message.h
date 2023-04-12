#pragma once

#include "messages/jandy/messages/jandy_message.h"

namespace AqualinkAutomate::Messages::Jandy::Messages
{

	class JandyMessageMessage : public JandyMessage
	{
	public:
		JandyMessageMessage();
		virtual ~JandyMessageMessage();

	public:
		void Deserialize(const std::span<const std::byte>& message_bytes);
	};

}
// namespace AqualinkAutomate::Messages::Jandy::Messages
