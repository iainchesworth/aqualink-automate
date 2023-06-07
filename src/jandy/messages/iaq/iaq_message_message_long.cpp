#include <format>

#include "jandy/messages/iaq/iaq_message_message_long.h"
#include "jandy/messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::IAQMessage_MessageLong> IAQMessage_MessageLong::g_IAQMessage_MessageLong_Registration(JandyMessageIds::IAQ_MessageLong);

	IAQMessage_MessageLong::IAQMessage_MessageLong() : 
		IAQMessage(JandyMessageIds::IAQ_MessageLong),
		Interfaces::IMessageSignalRecv<IAQMessage_MessageLong>()
	{
	}

	IAQMessage_MessageLong::~IAQMessage_MessageLong()
	{
	}

	std::string IAQMessage_MessageLong::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	void IAQMessage_MessageLong::Serialize(std::vector<uint8_t>& message_bytes) const
	{
	}

	void IAQMessage_MessageLong::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_MessageLong type", message_bytes.size()));

			IAQMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
