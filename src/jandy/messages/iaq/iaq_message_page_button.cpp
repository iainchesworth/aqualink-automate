#include <format>

#include "logging/logging.h"
#include "jandy/messages/iaq/iaq_message_page_button.h"
#include "jandy/messages/jandy_message_ids.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	IAQMessage_PageButton::IAQMessage_PageButton() : 
		IAQMessage(JandyMessageIds::IAQ_PageButton),
		Interfaces::IMessageSignal<IAQMessage_PageButton>()
	{
	}

	IAQMessage_PageButton::~IAQMessage_PageButton()
	{
	}

	std::string IAQMessage_PageButton::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	void IAQMessage_PageButton::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void IAQMessage_PageButton::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_PageButton type", message_bytes.size()));

			IAQMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
