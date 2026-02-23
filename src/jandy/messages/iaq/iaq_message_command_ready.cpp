#include <format>

#include "messages/iaq/iaq_message_command_ready.h"
#include "messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	IAQMessage_CommandReady::IAQMessage_CommandReady() noexcept :
		IAQMessage(JandyMessageIds::IAQ_CommandReady),
		Interfaces::IMessageSignalRecv<IAQMessage_CommandReady>()
	{
	}


	std::string IAQMessage_CommandReady::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	bool IAQMessage_CommandReady::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool IAQMessage_CommandReady::DeserializeContents(std::span<const uint8_t> message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_CommandReady type", message_bytes.size()));

		return true;
	}

}
// namespace AqualinkAutomate::Messages
