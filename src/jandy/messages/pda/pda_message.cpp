#include <format>

#include "jandy/messages/pda/pda_message.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	PDAMessage::PDAMessage(const JandyMessageIds& msg_id) : JandyMessage(msg_id)
	{
	}

	PDAMessage::~PDAMessage()
	{
	}

	std::string PDAMessage::ToString() const
	{
		return JandyMessage::ToString();
	}

	void PDAMessage::Serialize(std::vector<uint8_t>& message_bytes) const
	{
	}

	void PDAMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into PDAMessage type", message_bytes.size()));

			JandyMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
