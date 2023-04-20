#include <format>

#include "logging/logging.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/aquarite/aquarite_message_percent.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging; 

namespace AqualinkAutomate::Messages
{

	AquariteMessage_Percent::AquariteMessage_Percent() : 
		AquariteMessage(JandyMessageIds::AQUARITE_Percent),
		Interfaces::IMessageSignal<AquariteMessage_Percent>(),
		m_Percent(0)
	{
	}

	AquariteMessage_Percent::~AquariteMessage_Percent()
	{
	}

	uint8_t AquariteMessage_Percent::GeneratingPercentage() const
	{
		return m_Percent;
	}

	std::string AquariteMessage_Percent::ToString() const
	{
		return std::format("Packet: {} || Payload: Percent: {}", AquariteMessage::ToString(), m_Percent);
	}

	void AquariteMessage_Percent::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void AquariteMessage_Percent::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into AquariteMessage_Percent type", message_bytes.size()));

			m_Percent = static_cast<uint8_t>(message_bytes[Index_Percent]);

			AquariteMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7 - 1));
		}
	}

}
// namespace AqualinkAutomate::Messages
