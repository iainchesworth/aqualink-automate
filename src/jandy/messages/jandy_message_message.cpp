#include <algorithm>
#include <format>

#include <magic_enum/magic_enum.hpp>

#include "messages/jandy_message_ids.h"
#include "messages/jandy_message_message.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	//const Factory::JandyMessageRegistration<Messages::JandyMessage_Message> JandyMessage_Message::g_JandyMessage_Message_Registration(JandyMessageIds::Message);

	JandyMessage_Message::JandyMessage_Message() noexcept : 
		JandyMessage_Message(std::string())
	{
		m_Line.reserve(MAXIMUM_MESSAGE_LENGTH);
	}

	JandyMessage_Message::JandyMessage_Message(const std::string& line) :
		JandyMessage(JandyMessageIds::Message),
		Interfaces::IMessageSignalRecv<JandyMessage_Message>(),
		m_Line(line)
	{
	}

	JandyMessage_Message::~JandyMessage_Message() = default;

	std::string JandyMessage_Message::Line() const
	{
		return m_Line;
	}

	std::string JandyMessage_Message::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	bool JandyMessage_Message::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		message_bytes.reserve(message_bytes.size() + m_Line.size());

		for (auto& elem : m_Line)
		{
			message_bytes.emplace_back(static_cast<uint8_t>(elem));
		}

		return true;
	}

	bool JandyMessage_Message::DeserializeContents(std::span<const uint8_t> message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyMessage_Message type", message_bytes.size()));

		if (message_bytes.size() <= Index_LineText)
		{
			LogDebug(Channel::Messages, "JandyMessage_Message is too short to deserialise LineText");
		}
		else if (static_cast<uint64_t>(JandyMessage::MINIMUM_PACKET_LENGTH + 1) > message_bytes.size())
		{
			LogDebug(Channel::Messages, "JandyMessage_Message is too short to deserialise content of LineText");
		}
		else if (message_bytes.size() < Index_LineText + 3)
		{
			// Security: Prevent integer underflow in length calculation
			LogDebug(Channel::Messages, "JandyMessage_Message is too short for content extraction");
		}
		else
		{
			const auto payload_length = message_bytes.size() - Index_LineText - 3;
			const auto length_to_copy = std::min<std::size_t>(payload_length, DISPLAY_LINE_LENGTH);
			const auto start_index = message_bytes.begin() + Index_LineText;
			const auto end_index = start_index + length_to_copy;

			std::transform(start_index, end_index, std::back_inserter(m_Line),
				[](const auto& elem)
				{
					return static_cast<char>(elem);
				}
			);

			LogDebug(Channel::Messages, std::format("Deserialised JandyMessage_Message: LineText -> '{}' ({} chars)", m_Line, m_Line.length()));

			return true;
		}

		return false;
	}

}
// namespace AqualinkAutomate::Messages
