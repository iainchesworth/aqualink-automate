#pragma once

#include "messages/jandy/messages/jandy_message.h"

namespace AqualinkAutomate::Messages::Jandy::Messages
{

	class JandyStatusMessage : public JandyMessage
	{
	public:
		JandyStatusMessage();
		virtual ~JandyStatusMessage();

	public:
		void Deserialize(const std::span<const std::byte>& message_bytes);
	};

}
// namespace AqualinkAutomate::Messages::Jandy::Messages
