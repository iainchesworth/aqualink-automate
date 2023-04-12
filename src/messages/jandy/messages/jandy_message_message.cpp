#include <format>

#include "logging/logging.h"
#include "messages/jandy/messages/jandy_message_message.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages::Jandy::Messages
{
	JandyMessageMessage::JandyMessageMessage() : JandyMessage()
	{
	}

	JandyMessageMessage::~JandyMessageMessage()
	{
	}

	void JandyMessageMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyMessageMessage type", message_bytes.size()));
	}

}
// namespace AqualinkAutomate::Messages::Jandy::Messages
