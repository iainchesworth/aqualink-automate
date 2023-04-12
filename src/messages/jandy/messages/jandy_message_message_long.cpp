#include <format>

#include "logging/logging.h"
#include "messages/jandy/messages/jandy_message_message_long.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages::Jandy::Messages
{
	JandyMessageLongMessage::JandyMessageLongMessage() : JandyMessage()
	{
	}

	JandyMessageLongMessage::~JandyMessageLongMessage()
	{
	}

	void JandyMessageLongMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyMessageLongMessage type", message_bytes.size()));
	}

}
// namespace AqualinkAutomate::Messages::Jandy::Messages
