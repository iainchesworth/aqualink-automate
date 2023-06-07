#include <format>

#include "jandy/messages/iaq/iaq_message_poll.h"
#include "jandy/messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::IAQMessage_Poll> IAQMessage_Poll::g_IAQMessage_Poll_Registration(JandyMessageIds::IAQ_Poll);

	IAQMessage_Poll::IAQMessage_Poll() : 
		IAQMessage(JandyMessageIds::IAQ_Poll),
		Interfaces::IMessageSignalRecv<IAQMessage_Poll>()
	{
	}

	IAQMessage_Poll::~IAQMessage_Poll()
	{
	}

	std::string IAQMessage_Poll::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	void IAQMessage_Poll::Serialize(std::vector<uint8_t>& message_bytes) const
	{
	}

	void IAQMessage_Poll::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_Poll type", message_bytes.size()));

			IAQMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
