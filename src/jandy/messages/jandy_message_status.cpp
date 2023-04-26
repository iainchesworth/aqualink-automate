#include <format>

#include "logging/logging.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_status.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::JandyMessage_Status> JandyMessage_Status::g_JandyMessage_Status_Registration(JandyMessageIds::Status);

	JandyMessage_Status::JandyMessage_Status() : 
		JandyMessage(JandyMessageIds::Status),
		Interfaces::IMessageSignal<JandyMessage_Status>()
	{
	}

	JandyMessage_Status::~JandyMessage_Status()
	{
	}

	std::string JandyMessage_Status::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	void JandyMessage_Status::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void JandyMessage_Status::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyMessage_Status type", message_bytes.size()));

			JandyMessage::Deserialize(message_bytes);

			LogDebug(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
