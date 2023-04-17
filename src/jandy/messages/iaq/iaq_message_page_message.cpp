#include <format>

#include "logging/logging.h"
#include "jandy/messages/iaq/iaq_message_page_message.h"
#include "jandy/messages/jandy_message_ids.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	IAQMessage_PageMessage::IAQMessage_PageMessage() : 
		IAQMessage(JandyMessageIds::IAQ_PageMessage)
	{
	}

	IAQMessage_PageMessage::~IAQMessage_PageMessage()
	{
	}

	std::string IAQMessage_PageMessage::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	void IAQMessage_PageMessage::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void IAQMessage_PageMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_PageMessage type", message_bytes.size()));

			IAQMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
