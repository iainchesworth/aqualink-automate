#include <format>

#include "logging/logging.h"
#include "messages/jandy/messages/jandy_message_probe.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages::Jandy::Messages
{
	JandyProbeMessage::JandyProbeMessage() : JandyMessage()
	{
	}

	JandyProbeMessage::~JandyProbeMessage()
	{
	}

	void JandyProbeMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyProbeMessage type", message_bytes.size()));
	}

}
// namespace AqualinkAutomate::Messages::Jandy::Messages
