#include <format>

#include "messages/iaq/iaq_message_message_long.h"
#include "messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	IAQMessage_MessageLong::IAQMessage_MessageLong() noexcept :
		IAQMessage(JandyMessageIds::IAQ_MessageLong),
		Interfaces::IMessageSignalRecv<IAQMessage_MessageLong>()
	{
	}


	std::string IAQMessage_MessageLong::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	bool IAQMessage_MessageLong::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool IAQMessage_MessageLong::DeserializeContents(std::span<const uint8_t> message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_MessageLong type", message_bytes.size()));

		return true;
	}

}
// namespace AqualinkAutomate::Messages
