#include <format>

#include <magic_enum.hpp>

#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/aquarite/aquarite_message_ppm.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging; 

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::AquariteMessage_PPM> AquariteMessage_PPM::g_AquariteMessage_PPM_Registration(JandyMessageIds::AQUARITE_PPM);

	AquariteMessage_PPM::AquariteMessage_PPM() : 
		AquariteMessage(JandyMessageIds::AQUARITE_PPM),
		Interfaces::IMessageSignalRecv<AquariteMessage_PPM>(),
		m_PPM(0),
		m_Status(AquariteStatuses::Unknown)
	{
	}

	AquariteMessage_PPM::~AquariteMessage_PPM()
	{
	}

	uint16_t AquariteMessage_PPM::SaltConcentrationPPM() const
	{
		return m_PPM;
	}

	AquariteStatuses AquariteMessage_PPM::Status() const
	{
		return m_Status;
	}

	std::string AquariteMessage_PPM::ToString() const
	{
		return std::format("Packet: {} || Payload: PPM: {}, Status: {}", AquariteMessage::ToString(), m_PPM, magic_enum::enum_name(m_Status));
	}

	void AquariteMessage_PPM::Serialize(std::vector<uint8_t>& message_bytes) const
	{
	}

	void AquariteMessage_PPM::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into AquariteMessage_PPM type", message_bytes.size()));

			m_PPM = static_cast<uint16_t>(message_bytes[Index_PPM]) * 100;
			m_Status = magic_enum::enum_cast<AquariteStatuses>(static_cast<uint8_t>(message_bytes[Index_Status])).value_or(AquariteStatuses::Unknown);

			AquariteMessage::Deserialize(message_bytes);

			LogTrace(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7 - 2));
		}
	}

}
// namespace AqualinkAutomate::Messages
