#include <format>

#include "messages/iaq/iaq_message_control_ready.h"
#include "messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	IAQMessage_ControlReady::IAQMessage_ControlReady() noexcept :
		IAQMessage(JandyMessageIds::IAQ_ControlReady),
		Interfaces::IMessageSignalRecv<IAQMessage_ControlReady>()
	{
	}

	IAQMessage_ControlReady::~IAQMessage_ControlReady() = default;

	std::string IAQMessage_ControlReady::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	bool IAQMessage_ControlReady::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool IAQMessage_ControlReady::DeserializeContents(std::span<const uint8_t> message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_ControlReady type", message_bytes.size()));

		return true;
	}

}
// namespace AqualinkAutomate::Messages
