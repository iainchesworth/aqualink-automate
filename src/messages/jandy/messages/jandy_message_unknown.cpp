#include <format>

#include "logging/logging.h"
#include "messages/jandy/messages/jandy_message_unknown.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages::Jandy::Messages
{

	JandyUnknownMessage::JandyUnknownMessage() : JandyMessage() 
	{
	}

	JandyUnknownMessage::~JandyUnknownMessage() 
	{
	}

	std::string JandyUnknownMessage::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	void JandyUnknownMessage::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void JandyUnknownMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyUnknownMessage type", message_bytes.size()));

			JandyMessage::Deserialize(message_bytes);

			LogDebug(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages::Jandy::Messages
