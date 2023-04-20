#include <format>

#include "logging/logging.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_ids.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{
	JandyAckMessage::JandyAckMessage() : 
		JandyMessage(JandyMessageIds::Ack),
		Interfaces::IMessageSignal<JandyAckMessage>()
	{
	}

	JandyAckMessage::~JandyAckMessage()
	{
	}

	std::string JandyAckMessage::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	void JandyAckMessage::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void JandyAckMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyAckMessage type", message_bytes.size()));

			JandyMessage::Deserialize(message_bytes);

			LogDebug(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
