#include <format>

#include "logging/logging.h"
#include "messages/jandy/messages/aquarite/aquarite_message.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages::Jandy::Messages::Aquarite
{

	AquariteMessage::AquariteMessage() : JandyMessage()
	{
	}

	AquariteMessage::~AquariteMessage()
	{
	}

	std::string AquariteMessage::ToString() const
	{
		return JandyMessage::ToString();
	}

	void AquariteMessage::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void AquariteMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into AquariteMessage type", message_bytes.size()));

			JandyMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages::Jandy::Messages::Aquarite
