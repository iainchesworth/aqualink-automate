#include <format>

#include "logging/logging.h"
#include "messages/jandy/messages/jandy_message_status.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages::Jandy::Messages
{
	JandyStatusMessage::JandyStatusMessage() : JandyMessage()
	{
	}

	JandyStatusMessage::~JandyStatusMessage()
	{
	}

	std::string JandyStatusMessage::Print() const
	{
		std::string printable_output{};
		printable_output = JandyMessage::Print();
		return printable_output;
	}

	void JandyStatusMessage::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void JandyStatusMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyStatusMessage type", message_bytes.size()));

		JandyMessage::Deserialize(message_bytes);

		LogDebug(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
	}

}
// namespace AqualinkAutomate::Messages::Jandy::Messages
