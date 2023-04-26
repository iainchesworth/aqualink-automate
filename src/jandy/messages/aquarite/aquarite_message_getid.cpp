#include <format>

#include "logging/logging.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/aquarite/aquarite_message_getid.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::AquariteMessage_GetId> AquariteMessage_GetId::g_AquariteMessage_GetId_Registration(JandyMessageIds::AQUARITE_GetId);

	AquariteMessage_GetId::AquariteMessage_GetId() : 
		AquariteMessage(JandyMessageIds::AQUARITE_GetId),
		Interfaces::IMessageSignal<AquariteMessage_GetId>()
	{
	}

	AquariteMessage_GetId::~AquariteMessage_GetId()
	{
	}

	std::string AquariteMessage_GetId::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", AquariteMessage::ToString(), 0);
	}

	void AquariteMessage_GetId::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void AquariteMessage_GetId::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into AquariteMessage_GetId type", message_bytes.size()));

			AquariteMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
