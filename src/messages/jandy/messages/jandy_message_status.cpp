#include <format>

#include "logging/logging.h"
#include "messages/jandy/messages/jandy_message_status.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages::Jandy::Messages
{
	JandyStatusMessage::JandyStatusMessage() : JandyMessage()
	{
	}

	JandyStatusMessage::~JandyStatusMessage()
	{
	}

	void JandyStatusMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyStatusMessage type", message_bytes.size()));
	}

}
// namespace AqualinkAutomate::Messages::Jandy::Messages
