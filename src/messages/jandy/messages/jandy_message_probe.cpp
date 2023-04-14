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

	std::string JandyProbeMessage::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	void JandyProbeMessage::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void JandyProbeMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyProbeMessage type", message_bytes.size()));

			JandyMessage::Deserialize(message_bytes);
		}
	}

}
// namespace AqualinkAutomate::Messages::Jandy::Messages
