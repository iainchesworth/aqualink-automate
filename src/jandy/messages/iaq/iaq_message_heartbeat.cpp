#include <format>

#include "messages/iaq/iaq_message_heartbeat.h"
#include "messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	IAQMessage_Heartbeat::IAQMessage_Heartbeat() noexcept :
		IAQMessage(JandyMessageIds::IAQ_Heartbeat),
		Interfaces::IMessageSignalRecv<IAQMessage_Heartbeat>()
	{
	}

	IAQMessage_Heartbeat::~IAQMessage_Heartbeat() = default;

	std::string IAQMessage_Heartbeat::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	bool IAQMessage_Heartbeat::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool IAQMessage_Heartbeat::DeserializeContents(std::span<const uint8_t> message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_Heartbeat type", message_bytes.size()));

		return true;
	}

}
// namespace AqualinkAutomate::Messages
