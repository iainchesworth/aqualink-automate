#include <format>

#include "jandy/messages/iaq/iaq_message_startup.h"
#include "jandy/messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::IAQMessage_StartUp> IAQMessage_StartUp::g_IAQMessage_StartUp_Registration(JandyMessageIds::IAQ_StartUp);

	IAQMessage_StartUp::IAQMessage_StartUp() : 
		IAQMessage(JandyMessageIds::IAQ_StartUp),
		Interfaces::IMessageSignalRecv<IAQMessage_StartUp>()
	{
	}

	IAQMessage_StartUp::~IAQMessage_StartUp()
	{
	}

	std::string IAQMessage_StartUp::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	void IAQMessage_StartUp::Serialize(std::vector<uint8_t>& message_bytes) const
	{
	}

	void IAQMessage_StartUp::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_StartUp type", message_bytes.size()));

			IAQMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
