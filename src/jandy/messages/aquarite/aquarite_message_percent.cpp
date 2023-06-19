#include <format>

#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/aquarite/aquarite_message_percent.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging; 

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::AquariteMessage_Percent> AquariteMessage_Percent::g_AquariteMessage_Percent_Registration(JandyMessageIds::AQUARITE_Percent);

	AquariteMessage_Percent::AquariteMessage_Percent() : 
		AquariteMessage(JandyMessageIds::AQUARITE_Percent),
		Interfaces::IMessageSignalRecv<AquariteMessage_Percent>(),
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

	bool AquariteMessage_Percent::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool AquariteMessage_Percent::DeserializeContents(const std::vector<uint8_t>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into AquariteMessage_Percent type", message_bytes.size()));

		if (message_bytes.size() < Index_Percent)
		{
			LogDebug(Channel::Messages, "AquariteMessage_Percent is too short to deserialise Percent.");
		}
		else
		{
			m_Percent = static_cast<uint8_t>(message_bytes[Index_Percent]);
			return true;
		}

		return false;
	}

}
// namespace AqualinkAutomate::Messages
