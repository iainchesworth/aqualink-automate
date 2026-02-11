#include <format>

#include "messages/iaq/iaq_message_startup.h"
#include "messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	IAQMessage_StartUp::IAQMessage_StartUp() noexcept :
		IAQMessage(JandyMessageIds::IAQ_StartUp),
		Interfaces::IMessageSignalRecv<IAQMessage_StartUp>()
	{
	}

	IAQMessage_StartUp::~IAQMessage_StartUp() = default;

	std::string IAQMessage_StartUp::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	bool IAQMessage_StartUp::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool IAQMessage_StartUp::DeserializeContents(std::span<const uint8_t> message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_StartUp type", message_bytes.size()));

		return true;
	}

}
// namespace AqualinkAutomate::Messages
