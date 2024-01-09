#include <format>

#include <magic_enum.hpp>

#include "jandy/messages/jandy_message_constants.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/aquarite/aquarite_message_ppm.h"
#include "jandy/utility/jandy_checksum.h"
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

	bool AquariteMessage_PPM::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		message_bytes =
		{
			Messages::HEADER_BYTE_DLE,
			Messages::HEADER_BYTE_STX,
			0x00,
			magic_enum::enum_integer(JandyMessageIds::AQUARITE_PPM),
			static_cast<uint8_t>(m_PPM / 100),
			magic_enum::enum_integer(m_Status),
			0x00,
			Messages::HEADER_BYTE_DLE,
			Messages::HEADER_BYTE_ETX
		};

		auto message_span_to_checksum = std::as_bytes(std::span<uint8_t>(message_bytes.begin(), 6));
		message_bytes[6] = Utility::JandyPacket_CalculateChecksum(message_span_to_checksum);

		return true;
	}

	bool AquariteMessage_PPM::DeserializeContents(const std::vector<uint8_t>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into AquariteMessage_PPM type", message_bytes.size()));

		if (message_bytes.size() <= Index_PPM)
		{
			LogDebug(Channel::Messages, "AquariteMessage_PPM is too short to deserialise PPM.");
		}
		else if (message_bytes.size() <= Index_Status)
		{
			LogDebug(Channel::Messages, "AquariteMessage_PPM is too short to deserialise Status.");
		}
		else
		{
			m_PPM = static_cast<uint16_t>(message_bytes[Index_PPM]) * 100;
			m_Status = magic_enum::enum_cast<AquariteStatuses>(static_cast<uint8_t>(message_bytes[Index_Status])).value_or(AquariteStatuses::Unknown);

			return true;
		}

		return false;
	}

}
// namespace AqualinkAutomate::Messages
