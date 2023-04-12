#include <format>

#include "logging/logging.h"
#include "messages/jandy/messages/jandy_message_unknown.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages::Jandy::Messages
{
	JandyUnknownMessage::JandyUnknownMessage() : JandyMessage() 
	{
	}

	JandyUnknownMessage::~JandyUnknownMessage() 
	{
	}

	void JandyUnknownMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyUnknownMessage type", message_bytes.size()));
	}

}
// namespace AqualinkAutomate::Messages::Jandy::Messages
