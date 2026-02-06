#include <format>

#include <magic_enum/magic_enum.hpp>

#include "messages/aquarite/aquarite_message_getid.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	AquariteMessage_GetId::AquariteMessage_GetId() noexcept :
		AquariteMessage_GetId(PanelDataTypes::Unknown)
	{
	}

	AquariteMessage_GetId::AquariteMessage_GetId(PanelDataTypes requested_panel_data) :
		AquariteMessage(JandyMessageIds::AQUARITE_GetId),
		Interfaces::IMessageSignalRecv<AquariteMessage_GetId>(),
		m_RequestedPanelData(requested_panel_data)
	{
	}

	AquariteMessage_GetId::~AquariteMessage_GetId() = default;

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
		message_bytes.emplace_back(magic_enum::enum_integer(m_RequestedPanelData));

		return true;
	}

	bool AquariteMessage_GetId::DeserializeContents(const std::vector<uint8_t>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into AquariteMessage_GetId type", message_bytes.size()));

		if (message_bytes.size() <= Index_RequestedDataFlag)
		{
			LogDebug(Channel::Messages, "AquariteMessage_GetId is too short to deserialise RequestedPanelData.");
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
