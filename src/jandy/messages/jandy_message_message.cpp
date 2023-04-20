#include <format>

#include "logging/logging.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_message.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	JandyMessageMessage::JandyMessageMessage() : 
		JandyMessage(JandyMessageIds::Message),
		Interfaces::IMessageSignal<JandyMessageMessage>()
	{
	}

	JandyMessageMessage::~JandyMessageMessage()
	{
	}

	std::string JandyMessageMessage::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	void JandyMessageMessage::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void JandyMessageMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyMessageMessage type", message_bytes.size()));

			JandyMessage::Deserialize(message_bytes);

			LogDebug(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
