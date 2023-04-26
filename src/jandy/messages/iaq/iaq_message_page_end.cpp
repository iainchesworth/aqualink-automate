#include <format>

#include "logging/logging.h"
#include "jandy/messages/iaq/iaq_message_page_end.h"
#include "jandy/messages/jandy_message_ids.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::IAQMessage_PageEnd> IAQMessage_PageEnd::g_IAQMessage_PageEnd_Registration(JandyMessageIds::IAQ_PageEnd);

	IAQMessage_PageEnd::IAQMessage_PageEnd() : 
		IAQMessage(JandyMessageIds::IAQ_PageEnd),
		Interfaces::IMessageSignal<IAQMessage_PageEnd>()
	{
	}

	IAQMessage_PageEnd::~IAQMessage_PageEnd()
	{
	}

	std::string IAQMessage_PageEnd::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	void IAQMessage_PageEnd::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void IAQMessage_PageEnd::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_PageEnd type", message_bytes.size()));

			IAQMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
