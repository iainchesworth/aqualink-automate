#include <format>

#include "messages/iaq/iaq_message_title_message.h"
#include "messages/jandy_message_ids.h"
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

	IAQMessage_TitleMessage::~IAQMessage_TitleMessage() = default;

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

	bool IAQMessage_TitleMessage::DeserializeContents(const std::vector<uint8_t>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_TitleMessage type", message_bytes.size()));

		if (message_bytes.size() <= Index_TitleText)
		{
			LogDebug(Channel::Messages, "IAQMessage_TitleMessage is too short to deserialise Title.");
		}
		else if (static_cast<uint64_t>(JandyMessage::MINIMUM_PACKET_LENGTH + 1) > message_bytes.size())
		{
			LogDebug(Channel::Messages, "IAQMessage_TitleMessage is too short to deserialise content of Title");
		}
		else if (message_bytes.size() < Index_TitleText + 3)
		{
			// Security: Prevent integer underflow in length calculation
			LogDebug(Channel::Messages, "IAQMessage_TitleMessage is too short for content extraction");
		}
		else
		{
			const auto length_to_copy = message_bytes.size() - Index_TitleText - 3;
			const auto start_index = message_bytes.begin() + Index_TitleText;
			const auto end_index = start_index + length_to_copy;

			std::transform(start_index, end_index, std::back_inserter(m_Title),
				[](const auto& elem)
				{
					return static_cast<char>(elem);
				}
			);

			LogDebug(Channel::Messages, std::format("Deserialised IAQMessage_TitleMessage: Title -> '{}' ({} chars)", m_Title, m_Title.length()));

			return true;
		}

		return false;
	}

}
// namespace AqualinkAutomate::Messages
