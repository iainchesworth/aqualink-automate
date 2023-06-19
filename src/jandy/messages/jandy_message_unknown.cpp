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

	bool JandyMessage_Unknown::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		LogTrace(Channel::Messages, std::format("Serialising JandyMessage_Unknown type into {} bytes", message_bytes.size()));

		return true;
	}

	bool JandyMessage_Unknown::DeserializeContents(const std::vector<uint8_t>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes into JandyMessage_Unknown type", message_bytes.size()));

		return true;
	}

}
// namespace AqualinkAutomate::Messages
