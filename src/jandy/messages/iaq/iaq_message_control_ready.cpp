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

	void IAQMessage_ControlReady::Serialize(std::vector<uint8_t>& message_bytes) const
	{
	}

	void IAQMessage_ControlReady::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_ControlReady type", message_bytes.size()));

			IAQMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
