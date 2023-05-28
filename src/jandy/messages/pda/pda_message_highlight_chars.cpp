#include <format>

#include "logging/logging.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/pda/pda_message_highlight_chars.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::PDAMessage_HighlightChars> PDAMessage_HighlightChars::g_PDAMessage_HighlightChars_Registration(JandyMessageIds::PDA_HighlightChars);

	PDAMessage_HighlightChars::PDAMessage_HighlightChars() :
		PDAMessage(JandyMessageIds::PDA_HighlightChars),
		Interfaces::IMessageSignalRecv<PDAMessage_HighlightChars>(),
		m_LineId(0),
		m_StartIndex(0),
		m_StopIndex(0)
	{
	}

	PDAMessage_HighlightChars::~PDAMessage_HighlightChars()
	{
	}

	uint8_t PDAMessage_HighlightChars::LineId() const
	{
		return m_LineId;
	}

	uint8_t PDAMessage_HighlightChars::StartIndex() const
	{
		return m_StartIndex;
	}

	uint8_t PDAMessage_HighlightChars::StopIndex() const
	{
		return m_StopIndex;
	}

	std::string PDAMessage_HighlightChars::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", PDAMessage::ToString(), 0);
	}

	void PDAMessage_HighlightChars::Serialize(std::vector<uint8_t>& message_bytes) const
	{
	}

	void PDAMessage_HighlightChars::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into PDAMessage_HighlightChars type", message_bytes.size()));

			if (message_bytes.size() < Index_LineId)
			{
				LogDebug(Channel::Messages, "PDAMessage_HighlightChars is too short to deserialise LineId.");
			}
			else if (message_bytes.size() < Index_StartIndex)
			{
				LogDebug(Channel::Messages, "PDAMessage_HighlightChars is too short to deserialise StartIndex.");
			}
			else if (message_bytes.size() < Index_StopIndex)
			{
				LogDebug(Channel::Messages, "PDAMessage_HighlightChars is too short to deserialise StopIndex.");
			}
			else
			{
				m_LineId = static_cast<uint8_t>(message_bytes[Index_LineId]);
				m_StartIndex = static_cast<uint8_t>(message_bytes[Index_StartIndex]);
				m_StopIndex = static_cast<uint8_t>(message_bytes[Index_StopIndex]);
			}

			PDAMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
