#include <format>

#include "jandy/messages/jandy_message_constants.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/aquarite/aquarite_message_percent.h"
#include "jandy/utility/jandy_checksum.h"
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
		message_bytes =
		{
			Messages::HEADER_BYTE_DLE,
			Messages::HEADER_BYTE_STX,
			0x00,
			magic_enum::enum_integer(JandyMessageIds::AQUARITE_Percent),
			m_Percent,
			0x00,
			Messages::HEADER_BYTE_DLE,
			Messages::HEADER_BYTE_ETX
		};

		auto message_span_to_checksum = std::as_bytes(std::span<uint8_t>(message_bytes.begin(), 5));
		message_bytes[5] = Utility::JandyPacket_CalculateChecksum(message_span_to_checksum);

		return true;
	}

	bool AquariteMessage_Percent::DeserializeContents(const std::vector<uint8_t>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into AquariteMessage_Percent type", message_bytes.size()));

		if (message_bytes.size() <= Index_Percent)
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
