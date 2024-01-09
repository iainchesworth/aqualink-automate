#include <format>

#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_message_long.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::JandyMessage_MessageLong> JandyMessage_MessageLong::g_JandyMessage_MessageLong_Registration(JandyMessageIds::MessageLong);

	JandyMessage_MessageLong::JandyMessage_MessageLong() : 
		JandyMessage_MessageLong(0, std::string())
	{
		m_Line.reserve(MAXIMUM_MESSAGE_LENGTH);
	}

	JandyMessage_MessageLong::JandyMessage_MessageLong(const uint8_t line_id, const std::string line) :
		JandyMessage(JandyMessageIds::MessageLong),
		Interfaces::IMessageSignalRecv<JandyMessage_MessageLong>(),
		m_LineId(line_id),
		m_Line(line)
	{
		m_Line.reserve(MAXIMUM_MESSAGE_LENGTH);
	}

	JandyMessage_MessageLong::~JandyMessage_MessageLong()
	{
	}

	uint8_t JandyMessage_MessageLong::LineId() const
	{
		return m_LineId;
	}

	std::string JandyMessage_MessageLong::Line() const
	{
		return m_Line;
	}

	std::string JandyMessage_MessageLong::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	bool JandyMessage_MessageLong::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		message_bytes.reserve(message_bytes.size() + m_Line.size() + 1);

		message_bytes.emplace_back(static_cast<uint8_t>(m_LineId));

		for (auto& elem : m_Line)
		{
			message_bytes.emplace_back(static_cast<uint8_t>(elem));
		}

		return true;
	}

	bool JandyMessage_MessageLong::DeserializeContents(const std::vector<uint8_t>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyMessage_MessageLong type", message_bytes.size()));

		if (message_bytes.size() <= Index_LineId)
		{
			LogDebug(Channel::Messages, "JandyMessage_MessageLong is too short to deserialise LineId");
		}
		else if (message_bytes.size() <= Index_LineText)
		{
			LogDebug(Channel::Messages, "JandyMessage_MessageLong is too short to deserialise LineText");
		}
		else if (static_cast<uint64_t>(JandyMessage::MINIMUM_PACKET_LENGTH + 1 + 1) > message_bytes.size())
		{
			LogDebug(Channel::Messages, "JandyMessage_MessageLong is too short to deserialise content of LineText");
		}
		else
		{
			m_LineId = static_cast<uint8_t>(message_bytes[Index_LineId]);

			const auto length_to_copy = message_bytes.size() - Index_LineText - 3;
			const auto start_index = message_bytes.begin() + Index_LineText;
			const auto end_index = start_index + length_to_copy;;

			std::transform(start_index, end_index, std::back_inserter(m_Line),
				[](const auto& elem)
				{
					return static_cast<char>(elem);
				}
			);

			LogDebug(Channel::Messages, std::format("Deserialised JandyMessage_MessageLong: LineId -> {}, LineText -> '{}' ({} chars)", m_LineId, m_Line, m_Line.length()));

			return true;
		}

		return false;
	}

}
// namespace AqualinkAutomate::Messages
