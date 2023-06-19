#include <format>

#include "jandy/messages/iaq/iaq_message_control_ready.h"
#include "jandy/messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::IAQMessage_ControlReady> IAQMessage_ControlReady::g_IAQMessage_ControlReady_Registration(JandyMessageIds::IAQ_ControlReady);

	IAQMessage_ControlReady::IAQMessage_ControlReady() : 
		IAQMessage(JandyMessageIds::IAQ_ControlReady),
		Interfaces::IMessageSignalRecv<IAQMessage_ControlReady>()
	{
	}

	IAQMessage_ControlReady::~IAQMessage_ControlReady()
	{
	}

	std::string IAQMessage_ControlReady::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	bool IAQMessage_ControlReady::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool IAQMessage_ControlReady::DeserializeContents(const std::vector<uint8_t>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_ControlReady type", message_bytes.size()));

		return true;
	}

}
// namespace AqualinkAutomate::Messages
