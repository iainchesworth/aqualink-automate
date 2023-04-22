#include <format>

#include "logging/logging.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_message_long.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	JandyMessage_MessageLong::JandyMessage_MessageLong() : 
		JandyMessage(JandyMessageIds::MessageLong),
		Interfaces::IMessageSignal<JandyMessage_MessageLong>()
	{
	}

	JandyMessage_MessageLong::~JandyMessage_MessageLong()
	{
	}

	std::string JandyMessage_MessageLong::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	void JandyMessage_MessageLong::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void JandyMessage_MessageLong::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyMessage_MessageLong type", message_bytes.size()));

			JandyMessage::Deserialize(message_bytes);

			LogDebug(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
