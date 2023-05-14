#include <format>

#include "logging/logging.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_message_long.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::JandyMessage_MessageLong> JandyMessage_MessageLong::g_JandyMessage_MessageLong_Registration(JandyMessageIds::MessageLong);

	JandyMessage_MessageLong::JandyMessage_MessageLong() : 
		JandyMessage(JandyMessageIds::MessageLong),
		Interfaces::IMessageSignalRecv<JandyMessage_MessageLong>(),
		m_Line()
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

	void JandyMessage_MessageLong::Serialize(std::vector<uint8_t>& message_bytes) const
	{
	}

	void JandyMessage_MessageLong::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyMessage_MessageLong type", message_bytes.size()));

			if (message_bytes.size() < Index_LineId)
			{
				LogDebug(Channel::Messages, "JandyMessage_MessageLong is too short to deserialise LineId.");
			}
			else if (message_bytes.size() < Index_LineText)
			{
				LogDebug(Channel::Messages, "JandyMessage_MessageLong is too short to deserialise LineText.");
			}
			else if (0 >= (message_bytes.size() - 8))
			{
				LogDebug(Channel::Messages, "JandyMessage_MessageLong is too short to deserialise context of LineText.");
			}
			else
			{
				m_LineId = static_cast<uint8_t>(message_bytes[Index_LineId]);

				const auto length_to_copy = message_bytes.size() - Index_LineText - 3;
				for (auto& elem : message_bytes.subspan(Index_LineText, length_to_copy))
				{
					// Convert to char and push into the string.
					m_Line.push_back(static_cast<char>(elem));
				}

				LogDebug(Channel::Messages, std::format("Deserialised JandyMessage_MessageLong: LineId -> {}, LineText -> '{}' ({} chars)", m_LineId, m_Line, m_Line.length()));
			}

			JandyMessage::Deserialize(message_bytes);

			LogDebug(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
