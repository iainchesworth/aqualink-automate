#include <format>

#include "logging/logging.h"
#include "jandy/messages/iaq/iaq_message_page_message.h"
#include "jandy/messages/jandy_message_ids.h"

using namespace AqualinkAutomate;
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

	void IAQMessage_PageMessage::Serialize(std::vector<uint8_t>& message_bytes) const
	{
	}

	void IAQMessage_PageMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into IAQMessage_PageMessage type", message_bytes.size()));

			if (message_bytes.size() < Index_LineId)
			{
				LogDebug(Channel::Messages, "IAQMessage_PageMessage is too short to deserialise LineId.");
			}
			else if (message_bytes.size() < Index_LineText)
			{
				LogDebug(Channel::Messages, "IAQMessage_PageMessage is too short to deserialise LineText.");
			}
			else if (0 >= (message_bytes.size() - 8))
			{
				LogDebug(Channel::Messages, "IAQMessage_PageMessage is too short to deserialise context of LineText.");
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

				LogDebug(Channel::Messages, std::format("Deserialised IAQMessage_PageMessage: LineId -> {}, LineText -> '{}' ({} chars)", m_LineId, m_Line, m_Line.length()));
			}

			IAQMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 6));
		}
	}

}
// namespace AqualinkAutomate::Messages
