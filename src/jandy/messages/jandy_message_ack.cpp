#include <format>

#include "logging/logging.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_ids.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::JandyMessage_Ack> JandyMessage_Ack::g_JandyMessage_Ack_Registration(JandyMessageIds::Ack);

	JandyMessage_Ack::JandyMessage_Ack() : 
		JandyMessage(JandyMessageIds::Ack),
		Interfaces::IMessageSignal<JandyMessage_Ack>()
	{
	}

	JandyMessage_Ack::~JandyMessage_Ack()
	{
	}

	std::string JandyMessage_Ack::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	void JandyMessage_Ack::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void JandyMessage_Ack::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyMessage_Ack type", message_bytes.size()));

			JandyMessage::Deserialize(message_bytes);

			LogDebug(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
