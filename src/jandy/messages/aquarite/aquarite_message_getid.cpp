#include <format>

#include <magic_enum.hpp>

#include "jandy/messages/jandy_message_constants.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/aquarite/aquarite_message_getid.h"
#include "jandy/utility/jandy_checksum.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::AquariteMessage_GetId> AquariteMessage_GetId::g_AquariteMessage_GetId_Registration(JandyMessageIds::AQUARITE_GetId);

	AquariteMessage_GetId::AquariteMessage_GetId() :
		AquariteMessage_GetId(PanelDataTypes::Unknown)
	{
	}

	AquariteMessage_GetId::AquariteMessage_GetId(PanelDataTypes requested_panel_data) :
		AquariteMessage(JandyMessageIds::AQUARITE_GetId),
		Interfaces::IMessageSignalRecv<AquariteMessage_GetId>(),
		m_RequestedPanelData(requested_panel_data)
	{
	}

	AquariteMessage_GetId::~AquariteMessage_GetId()
	{
	}

	PanelDataTypes AquariteMessage_GetId::RequestedPanelData() const
	{
		return m_RequestedPanelData;
	}

	std::string AquariteMessage_GetId::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", AquariteMessage::ToString(), 0);
	}

	bool AquariteMessage_GetId::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		message_bytes =
		{
			Messages::HEADER_BYTE_DLE,
			Messages::HEADER_BYTE_STX,
			0x00,
			magic_enum::enum_integer(JandyMessageIds::AQUARITE_GetId),
			magic_enum::enum_integer(m_RequestedPanelData),
			0x00,
			Messages::HEADER_BYTE_DLE,
			Messages::HEADER_BYTE_ETX
		};

		auto message_span_to_checksum = std::as_bytes(std::span<uint8_t>(message_bytes.begin(), 5));
		message_bytes[5] = Utility::JandyPacket_CalculateChecksum(message_span_to_checksum);

		return true;
	}

	bool AquariteMessage_GetId::DeserializeContents(const std::vector<uint8_t>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into AquariteMessage_GetId type", message_bytes.size()));

		if (message_bytes.size() < Index_RequestedDataFlag)
		{
			LogDebug(Channel::Messages, "AquariteMessage_Percent is too short to deserialise RequestedPanelData.");
		}
		else
		{
			m_RequestedPanelData = magic_enum::enum_cast<PanelDataTypes>(static_cast<uint8_t>(message_bytes[Index_RequestedDataFlag])).value_or(PanelDataTypes::Unknown);

			return true;
		}

		return false;
	}

}
// namespace AqualinkAutomate::Messages
