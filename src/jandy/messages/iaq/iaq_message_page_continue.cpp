#include <format>

#include "jandy/messages/iaq/iaq_message_page_continue.h"
#include "jandy/messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::IAQMessage_PageContinue> IAQMessage_PageContinue::g_IAQMessage_PageContinue_Registration(JandyMessageIds::IAQ_PageContinue);

	IAQMessage_PageContinue::IAQMessage_PageContinue() : 
		IAQMessage(JandyMessageIds::IAQ_PageContinue),
		Interfaces::IMessageSignalRecv<IAQMessage_PageContinue>()
	{
	}

	IAQMessage_PageContinue::~IAQMessage_PageContinue()
	{
	}

	std::string IAQMessage_PageContinue::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	void IAQMessage_PageContinue::Serialize(std::vector<uint8_t>& message_bytes) const
	{
	}

	void IAQMessage_PageContinue::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_PageContinue type", message_bytes.size()));

			IAQMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
