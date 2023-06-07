#include <format>

#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_unknown.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{
	const Factory::JandyMessageRegistration<Messages::JandyMessage_Unknown> JandyMessage_Unknown::g_JandyMessage_Unknown_05_Registration(JandyMessageIds::Unknown_05);
	const Factory::JandyMessageRegistration<Messages::JandyMessage_Unknown> JandyMessage_Unknown::g_JandyMessage_Unknown_PDA_1B_Registration(JandyMessageIds::Unknown_PDA_1B);
	const Factory::JandyMessageRegistration<Messages::JandyMessage_Unknown> JandyMessage_Unknown::g_JandyMessage_Unknown_IAQ_2D_Registration(JandyMessageIds::Unknown_IAQ_2D);
	const Factory::JandyMessageRegistration<Messages::JandyMessage_Unknown> JandyMessage_Unknown::g_JandyMessage_Unknown_IAQ_70_Registration(JandyMessageIds::Unknown_IAQ_70);
	const Factory::JandyMessageRegistration<Messages::JandyMessage_Unknown> JandyMessage_Unknown::g_JandyMessage_Unknown_ReadyControl_Registration(JandyMessageIds::Unknown_ReadyControl);
	const Factory::JandyMessageRegistration<Messages::JandyMessage_Unknown> JandyMessage_Unknown::g_JandyMessage_Unknown_Registration(JandyMessageIds::Unknown);

	JandyMessage_Unknown::JandyMessage_Unknown() : 
		JandyMessage(JandyMessageIds::Unknown),
		Interfaces::IMessageSignalRecv<JandyMessage_Unknown>()
	{
	}

	JandyMessage_Unknown::~JandyMessage_Unknown() 
	{
	}

	std::string JandyMessage_Unknown::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	void JandyMessage_Unknown::Serialize(std::vector<uint8_t>& message_bytes) const
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
