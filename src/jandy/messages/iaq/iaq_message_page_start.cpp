#include <format>

#include "logging/logging.h"
#include "jandy/messages/iaq/iaq_message_page_start.h"
#include "jandy/messages/jandy_message_ids.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::IAQMessage_PageStart> IAQMessage_PageStart::g_IAQMessage_PageStart_Registration(JandyMessageIds::IAQ_PageStart);

	IAQMessage_PageStart::IAQMessage_PageStart() : 
		IAQMessage(JandyMessageIds::IAQ_PageStart),
		Interfaces::IMessageSignal<IAQMessage_PageStart>()
	{
	}

	IAQMessage_PageStart::~IAQMessage_PageStart()
	{
	}

	std::string IAQMessage_PageStart::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	void IAQMessage_PageStart::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void IAQMessage_PageStart::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_PageStart type", message_bytes.size()));

			IAQMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
