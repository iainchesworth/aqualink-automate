#include <format>

#include "logging/logging.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_unknown.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::JandyMessage_Unknown> JandyMessage_Unknown::g_JandyMessage_Unknown_Registration(JandyMessageIds::Unknown);

	JandyMessage_Unknown::JandyMessage_Unknown() : 
		JandyMessage(JandyMessageIds::Unknown),
		Interfaces::IMessageSignal<JandyMessage_Unknown>()
	{
	}

	JandyMessage_Unknown::~JandyMessage_Unknown() 
	{
	}

	std::string JandyMessage_Unknown::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	void JandyMessage_Unknown::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void JandyMessage_Unknown::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyMessage_Unknown type", message_bytes.size()));

			JandyMessage::Deserialize(message_bytes);

			LogDebug(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
