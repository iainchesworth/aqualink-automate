#include <format>

#include "messages/iaq/iaq_message_page_end.h"
#include "messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	IAQMessage_PageEnd::IAQMessage_PageEnd() noexcept :
		IAQMessage(JandyMessageIds::IAQ_PageEnd),
		Interfaces::IMessageSignalRecv<IAQMessage_PageEnd>()
	{
	}

	IAQMessage_PageEnd::~IAQMessage_PageEnd() = default;

	std::string IAQMessage_PageEnd::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	bool IAQMessage_PageEnd::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool IAQMessage_PageEnd::DeserializeContents(std::span<const uint8_t> message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_PageEnd type", message_bytes.size()));

		return true;
	}

}
// namespace AqualinkAutomate::Messages
