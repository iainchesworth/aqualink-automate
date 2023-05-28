#include <format>

#include "logging/logging.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/pda/pda_message_highlight.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::PDAMessage_Highlight> PDAMessage_Highlight::g_PDAMessage_Highlight_Registration(JandyMessageIds::PDA_Highlight);

	PDAMessage_Highlight::PDAMessage_Highlight() :
		PDAMessage(JandyMessageIds::PDA_Highlight),
		Interfaces::IMessageSignalRecv<PDAMessage_Highlight>(),
		m_LineId(0)
	{
	}

	PDAMessage_Highlight::~PDAMessage_Highlight()
	{
	}

	uint8_t PDAMessage_Highlight::LineId() const
	{
		return m_LineId;
	}

	std::string PDAMessage_Highlight::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", PDAMessage::ToString(), 0);
	}

	void PDAMessage_Highlight::Serialize(std::vector<uint8_t>& message_bytes) const
	{
	}

	void PDAMessage_Highlight::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into PDAMessage_Highlight type", message_bytes.size()));

			if (message_bytes.size() < Index_LineId)
			{
				LogDebug(Channel::Messages, "PDAMessage_Highlight is too short to deserialise LineId.");
			}
			else
			{
				m_LineId = static_cast<uint8_t>(message_bytes[Index_LineId]);
			}

			PDAMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
