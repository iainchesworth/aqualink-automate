#include <format>

#include "logging/logging.h"
#include "messages/jandy/messages/jandy_message_ack.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages::Jandy::Messages
{
	JandyAckMessage::JandyAckMessage() : JandyMessage()
	{
	}

	JandyAckMessage::~JandyAckMessage()
	{
	}

	void JandyAckMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyAckMessage type", message_bytes.size()));
	}

}
// namespace AqualinkAutomate::Messages::Jandy::Messages
