#include <cstddef>
#include <format>

#include "messages/iaq/iaq_message_title_message.h"
#include "messages/jandy_message_ids.h"
#include "messages/jandy_message_text_helpers.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	IAQMessage_TitleMessage::IAQMessage_TitleMessage() noexcept :
		IAQMessage(JandyMessageIds::IAQ_TitleMessage),
		Interfaces::IMessageSignalRecv<IAQMessage_TitleMessage>(),
		m_Title()
	{
	}


	std::string IAQMessage_TitleMessage::Title() const
	{
		return m_Title;
	}

	std::string IAQMessage_TitleMessage::ToString() const
	{
		return std::format("Packet: {} || Payload: Title: '{}'", IAQMessage::ToString(), m_Title);
	}

	bool IAQMessage_TitleMessage::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool IAQMessage_TitleMessage::DeserializeContents(std::span<const uint8_t> message_bytes)
	{
		LogTrace(Channel::Messages, [&]() { return std::format("Deserialising {} bytes from span into IAQMessage_TitleMessage type", message_bytes.size()); });

		// The trailing ASCII payload runs from Index_TitleText up to (but not
		// including) the 3-byte footer; ExtractTrailingAsciiPayload centralises
		// the underflow guard and sanitises wire-sourced text to printable ASCII.
		if (message_bytes.size() <= Index_TitleText + JandyMessage::PACKET_FOOTER_LENGTH)
		{
			LogDebug(Channel::Messages, "IAQMessage_TitleMessage is too short for content extraction");
			return false;
		}

		m_Title = Text::ExtractTrailingAsciiPayload(message_bytes, Index_TitleText);

		LogDebug(Channel::Messages, [&]() { return std::format("Deserialised IAQMessage_TitleMessage: Title -> '{}' ({} chars)", m_Title, m_Title.length()); });

		return true;
	}

}
// namespace AqualinkAutomate::Messages
