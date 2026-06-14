#include <format>

#include "messages/iaq/iaq_message_page_start.h"
#include "messages/jandy_message_ids.h"
#include "messages/jandy_message_text_helpers.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	IAQMessage_PageStart::IAQMessage_PageStart() noexcept :
		IAQMessage(JandyMessageIds::IAQ_PageStart),
		Interfaces::IMessageSignalRecv<IAQMessage_PageStart>()
	{
	}


	uint8_t IAQMessage_PageStart::PageId() const
	{
		return m_PageId;
	}

	std::string IAQMessage_PageStart::ToString() const
	{
		return std::format("Packet: {} || Payload: PageId: 0x{:02x}", IAQMessage::ToString(), m_PageId);
	}

	bool IAQMessage_PageStart::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool IAQMessage_PageStart::DeserializeContents(std::span<const uint8_t> message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_PageStart type", message_bytes.size()));

		// The page id is the first payload byte; tolerate a short frame (leave it 0 = unknown).
		if (Text::RequireIndex(message_bytes, Index_PageId, "IAQMessage_PageStart", "PageId"))
		{
			m_PageId = Text::ReadU8(message_bytes, Index_PageId);
		}

		return true;
	}

}
// namespace AqualinkAutomate::Messages
