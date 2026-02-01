#include <format>

#include "messages/iaq/iaq_message_page_start.h"
#include "messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	IAQMessage_PageStart::IAQMessage_PageStart() noexcept :
		IAQMessage(JandyMessageIds::IAQ_PageStart),
		Interfaces::IMessageSignalRecv<IAQMessage_PageStart>()
	{
	}

	IAQMessage_PageStart::~IAQMessage_PageStart()
	{
	}

	std::string IAQMessage_PageStart::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	bool IAQMessage_PageStart::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool IAQMessage_PageStart::DeserializeContents(const std::vector<uint8_t>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_PageStart type", message_bytes.size()));

		return true;
	}

}
// namespace AqualinkAutomate::Messages
