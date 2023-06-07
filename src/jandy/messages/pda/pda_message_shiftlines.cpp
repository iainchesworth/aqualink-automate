#include <format>

#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/pda/pda_message_shiftlines.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::PDAMessage_ShiftLines> PDAMessage_ShiftLines::g_PDAMessage_ShiftLines_Registration(JandyMessageIds::PDA_ShiftLines);

	PDAMessage_ShiftLines::PDAMessage_ShiftLines() :
		PDAMessage(JandyMessageIds::PDA_ShiftLines),
		Interfaces::IMessageSignalRecv<PDAMessage_ShiftLines>(),
		m_FirstLineId(0),
		m_LastLineId(0),
		m_LineShift(0)
	{
	}

	PDAMessage_ShiftLines::~PDAMessage_ShiftLines()
	{
	}

	uint8_t PDAMessage_ShiftLines::FirstLineId() const
	{
		return m_FirstLineId;
	}

	uint8_t PDAMessage_ShiftLines::LastLineId() const
	{
		return m_LastLineId;
	}

	int8_t PDAMessage_ShiftLines::LineShift() const
	{
		return m_LineShift;
	}

	std::string PDAMessage_ShiftLines::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", PDAMessage::ToString(), 0);
	}

	void PDAMessage_ShiftLines::Serialize(std::vector<uint8_t>& message_bytes) const
	{
	}

	void PDAMessage_ShiftLines::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into PDAMessage_ShiftLines type", message_bytes.size()));

			if (message_bytes.size() < Index_FirstLineId)
			{
				LogDebug(Channel::Messages, "PDAMessage_ShiftLines is too short to deserialise FirstLineId.");
			}
			else if (message_bytes.size() < Index_LastLineId)
			{
				LogDebug(Channel::Messages, "PDAMessage_ShiftLines is too short to deserialise LastLineId.");
			}
			else if (message_bytes.size() < Index_LineShift)
			{
				LogDebug(Channel::Messages, "PDAMessage_ShiftLines is too short to deserialise LineShift.");
			}
			else
			{
				m_FirstLineId = static_cast<uint8_t>(message_bytes[Index_FirstLineId]);
				m_LastLineId = static_cast<uint8_t>(message_bytes[Index_LastLineId]);
				m_LineShift = static_cast<int8_t>(message_bytes[Index_LineShift]);
			}

			PDAMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
