#include <format>

#include "logging/logging.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/pda/pda_message_shiftlines.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	PDAMessage_ShiftLines::PDAMessage_ShiftLines() :
		PDAMessage(JandyMessageIds::PDA_ShiftLines),
		Interfaces::IMessageSignal<PDAMessage_ShiftLines>()
	{
	}

	PDAMessage_ShiftLines::~PDAMessage_ShiftLines()
	{
	}

	std::string PDAMessage_ShiftLines::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", PDAMessage::ToString(), 0);
	}

	void PDAMessage_ShiftLines::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void PDAMessage_ShiftLines::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into PDAMessage_ShiftLines type", message_bytes.size()));

			PDAMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
