#include <format>

#include "jandy/messages/iaq/iaq_message_page_end.h"
#include "jandy/messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::IAQMessage_PageEnd> IAQMessage_PageEnd::g_IAQMessage_PageEnd_Registration(JandyMessageIds::IAQ_PageEnd);

	IAQMessage_PageEnd::IAQMessage_PageEnd() : 
		IAQMessage(JandyMessageIds::IAQ_PageEnd),
		Interfaces::IMessageSignalRecv<IAQMessage_PageEnd>()
	{
	}

	IAQMessage_PageEnd::~IAQMessage_PageEnd()
	{
	}

	std::string IAQMessage_PageEnd::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	bool IAQMessage_PageEnd::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool IAQMessage_PageEnd::DeserializeContents(const std::vector<uint8_t>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_PageEnd type", message_bytes.size()));

		return true;
	}

}
// namespace AqualinkAutomate::Messages
