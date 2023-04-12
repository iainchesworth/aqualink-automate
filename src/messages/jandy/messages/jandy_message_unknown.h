#pragma once

#include "messages/jandy/messages/jandy_message.h"

namespace AqualinkAutomate::Messages::Jandy::Messages
{

	class JandyUnknownMessage : public JandyMessage
	{
	public:
		JandyUnknownMessage();
		virtual ~JandyUnknownMessage();

	public:
		void Deserialize(const std::span<const std::byte>& message_bytes);
	};

}
// namespace AqualinkAutomate::Messages::Jandy::Messages
