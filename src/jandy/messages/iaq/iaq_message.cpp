#include <format>

#include "logging/logging.h"
#include "jandy/messages/iaq/iaq_message.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	IAQMessage::IAQMessage(const JandyMessageIds& msg_id) : JandyMessage(msg_id)
	{
	}

	IAQMessage::~IAQMessage()
	{
	}

	std::string IAQMessage::ToString() const
	{
		return JandyMessage::ToString();
	}

	void IAQMessage::Serialize(std::vector<uint8_t>& message_bytes) const
	{
	}

	void IAQMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage type", message_bytes.size()));

			JandyMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
