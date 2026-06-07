#include <cstddef>
#include <format>

#include "messages/iaq/iaq_message_table_message.h"
#include "messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	IAQMessage_TableMessage::IAQMessage_TableMessage() noexcept :
		IAQMessage(JandyMessageIds::IAQ_TableMessage),
		Interfaces::IMessageSignalRecv<IAQMessage_TableMessage>(),
		m_LineId(0),
		m_Line()
	{
	}


	uint8_t IAQMessage_TableMessage::LineId() const
	{
		return m_LineId;
	}

	std::string IAQMessage_TableMessage::Line() const
	{
		return m_Line;
	}

	std::string IAQMessage_TableMessage::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	bool IAQMessage_TableMessage::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool IAQMessage_TableMessage::DeserializeContents(std::span<const uint8_t> message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_TableMessage type", message_bytes.size()));

		if (message_bytes.size() <= Index_LineId)
		{
			LogDebug(Channel::Messages, "IAQMessage_TableMessage is too short to deserialise LineId.");
		}
		else if (message_bytes.size() <= Index_LineText)
		{
			LogDebug(Channel::Messages, "IAQMessage_TableMessage is too short to deserialise LineText.");
		}
		else if (static_cast<uint64_t>(JandyMessage::MINIMUM_PACKET_LENGTH + 1 + 1) > message_bytes.size())
		{
			LogDebug(Channel::Messages, "IAQMessage_TableMessage is too short to deserialise content of LineText");
		}
		else if (message_bytes.size() < Index_LineText + 3)
		{
			// Security: Prevent integer underflow in length calculation
			LogDebug(Channel::Messages, "IAQMessage_TableMessage is too short for content extraction");
		}
		else
		{
			m_LineId = static_cast<uint8_t>(message_bytes[Index_LineId]);

			const auto length_to_copy = message_bytes.size() - Index_LineText - 3;
			const auto start_index = message_bytes.begin() + Index_LineText;
			const auto end_index = start_index + static_cast<std::ptrdiff_t>(length_to_copy);

			m_Line.clear();
			std::transform(start_index, end_index, std::back_inserter(m_Line),
				[](const auto& elem)
				{
					return static_cast<char>(elem);
				}
			);

			LogDebug(Channel::Messages, std::format("Deserialised IAQMessage_TableMessage: LineId -> {}, LineText -> '{}' ({} chars)", m_LineId, m_Line, m_Line.length()));

			return true;
		}

		return false;
	}

}
// namespace AqualinkAutomate::Messages
