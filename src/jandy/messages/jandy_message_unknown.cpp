#include <format>

#include "messages/jandy_message_ids.h"
#include "messages/jandy_message_unknown.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{
	JandyMessage_Unknown::JandyMessage_Unknown() noexcept :
		JandyMessage(JandyMessageIds::Unknown),
		Interfaces::IMessageSignalRecv<JandyMessage_Unknown>()
	{
	}

	JandyMessage_Unknown::~JandyMessage_Unknown() = default;

	std::string JandyMessage_Unknown::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	bool JandyMessage_Unknown::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		LogTrace(Channel::Messages, std::format("Serialising JandyMessage_Unknown type into {} bytes", message_bytes.size()));

		return true;
	}

	bool JandyMessage_Unknown::DeserializeContents(const std::vector<uint8_t>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes into JandyMessage_Unknown type", message_bytes.size()));

		return true;
	}

}
// namespace AqualinkAutomate::Messages
