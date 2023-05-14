#include <format>

#include "logging/logging.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/pda/pda_message_clear.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::PDAMessage_Clear> PDAMessage_Clear::g_PDAMessage_Clear_Registration(JandyMessageIds::PDA_Clear);

	PDAMessage_Clear::PDAMessage_Clear() :
		PDAMessage(JandyMessageIds::PDA_Clear),
		Interfaces::IMessageSignalRecv<PDAMessage_Clear>()
	{
	}

	PDAMessage_Clear::~PDAMessage_Clear()
	{
	}

	std::string PDAMessage_Clear::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", PDAMessage::ToString(), 0);
	}

	void PDAMessage_Clear::Serialize(std::vector<uint8_t>& message_bytes) const
	{
	}

	void PDAMessage_Clear::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into PDAMessage_Clear type", message_bytes.size()));

			PDAMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
