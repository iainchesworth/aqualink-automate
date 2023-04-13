#include <format>

#include "logging/logging.h"
#include "messages/jandy/messages/jandy_message_message.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages::Jandy::Messages
{
	JandyMessageMessage::JandyMessageMessage() : JandyMessage()
	{
	}

	JandyMessageMessage::~JandyMessageMessage()
	{
	}

	std::string JandyMessageMessage::Print() const
	{
		std::string printable_output{};
		printable_output = JandyMessage::Print();
		return printable_output;
	}

	void JandyMessageMessage::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void JandyMessageMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyMessageMessage type", message_bytes.size()));

		JandyMessage::Deserialize(message_bytes);

		LogDebug(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
	}

}
// namespace AqualinkAutomate::Messages::Jandy::Messages
