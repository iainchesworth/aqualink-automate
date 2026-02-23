#include <format>

#include "messages/jandy_message_ids.h"
#include "messages/pda/pda_message_clear.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	PDAMessage_Clear::PDAMessage_Clear() noexcept :
		PDAMessage(JandyMessageIds::PDA_Clear),
		Interfaces::IMessageSignalRecv<PDAMessage_Clear>()
	{
	}


	std::string PDAMessage_Clear::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", PDAMessage::ToString(), 0);
	}

	bool PDAMessage_Clear::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool PDAMessage_Clear::DeserializeContents(std::span<const uint8_t> message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into PDAMessage_Clear type", message_bytes.size()));

		return true;
	}

}
// namespace AqualinkAutomate::Messages
