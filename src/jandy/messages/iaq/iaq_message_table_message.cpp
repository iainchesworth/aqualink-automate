#include <format>

#include "logging/logging.h"
#include "jandy/messages/iaq/iaq_message_table_message.h"
#include "jandy/messages/jandy_message_ids.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	IAQMessage_TableMessage::IAQMessage_TableMessage() : 
		IAQMessage(JandyMessageIds::IAQ_TableMessage)
	{
	}

	IAQMessage_TableMessage::~IAQMessage_TableMessage()
	{
	}

	std::string IAQMessage_TableMessage::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	void IAQMessage_TableMessage::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void IAQMessage_TableMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_TableMessage type", message_bytes.size()));

			IAQMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages