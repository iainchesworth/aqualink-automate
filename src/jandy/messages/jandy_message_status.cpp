#include <format>

#include "logging/logging.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_status.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	JandyStatusMessage::JandyStatusMessage() : 
		JandyMessage(JandyMessageIds::Status),
		Interfaces::IMessageSignal<JandyStatusMessage>()
	{
	}

	JandyStatusMessage::~JandyStatusMessage()
	{
	}

	std::string JandyStatusMessage::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	void JandyStatusMessage::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void JandyStatusMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyStatusMessage type", message_bytes.size()));

			JandyMessage::Deserialize(message_bytes);

			LogDebug(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
