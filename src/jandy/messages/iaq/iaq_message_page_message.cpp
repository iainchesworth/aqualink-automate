#include <format>

#include "jandy/messages/iaq/iaq_message_page_message.h"
#include "jandy/messages/jandy_message_ids.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::IAQMessage_PageMessage> IAQMessage_PageMessage::g_IAQMessage_PageMessage_Registration(JandyMessageIds::IAQ_PageMessage);

	IAQMessage_PageMessage::IAQMessage_PageMessage() : 
		IAQMessage(JandyMessageIds::IAQ_PageMessage),
		Interfaces::IMessageSignalRecv<IAQMessage_PageMessage>(),
		m_LineId(0),
		m_Line()
	{
	}

	IAQMessage_PageMessage::~IAQMessage_PageMessage()
	{
	}

	uint8_t IAQMessage_PageMessage::LineId() const
	{
		return m_LineId;
	}

	std::string IAQMessage_PageMessage::Line() const
	{
		return m_Line;
	}

	std::string IAQMessage_PageMessage::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", IAQMessage::ToString(), 0);
	}

	bool IAQMessage_PageMessage::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool IAQMessage_PageMessage::DeserializeContents(const std::vector<uint8_t>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_PageMessage type", message_bytes.size()));

		if (message_bytes.size() <= Index_LineId)
		{
			LogDebug(Channel::Messages, "IAQMessage_PageMessage is too short to deserialise LineId.");
		}
		else if (message_bytes.size() <= Index_LineText)
		{
			LogDebug(Channel::Messages, "IAQMessage_PageMessage is too short to deserialise LineText.");
		}
		else if (static_cast<uint64_t>(JandyMessage::MINIMUM_PACKET_LENGTH + 1 + 1) > message_bytes.size())
		{
			LogDebug(Channel::Messages, "IAQMessage_PageMessage is too short to deserialise content of LineText");
		}
		else
		{
			m_LineId = static_cast<uint8_t>(message_bytes[Index_LineId]);

			const auto length_to_copy = message_bytes.size() - Index_LineText - 3;
			const auto start_index = message_bytes.begin() + Index_LineText;
			const auto end_index = start_index + length_to_copy;

			std::transform(start_index, end_index, std::back_inserter(m_Line),
				[](const auto& elem)
				{
					return static_cast<char>(elem);
				}
			);

			LogDebug(Channel::Messages, std::format("Deserialised IAQMessage_PageMessage: LineId -> {}, LineText -> '{}' ({} chars)", m_LineId, m_Line, m_Line.length()));

			return true;
		}	

		return false;
	}

}
// namespace AqualinkAutomate::Messages
