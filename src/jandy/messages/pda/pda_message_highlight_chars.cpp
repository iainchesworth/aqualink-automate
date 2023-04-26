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
		Interfaces::IMessageSignal<PDAMessage_HighlightChars>()
	{
	}

	PDAMessage_HighlightChars::~PDAMessage_HighlightChars()
	{
	}

	std::string PDAMessage_HighlightChars::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", PDAMessage::ToString(), 0);
	}

	void PDAMessage_HighlightChars::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void PDAMessage_HighlightChars::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into PDAMessage_HighlightChars type", message_bytes.size()));

			PDAMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
