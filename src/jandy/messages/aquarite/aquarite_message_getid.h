#pragma once

#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/aquarite/aquarite_message.h"

namespace AqualinkAutomate::Messages
{

	enum class PanelDataTypes : uint8_t
	{
		PanelRevision = 0x01,
		PanelType = 0x02,
		Unknown = 0xFF
	};

	class AquariteMessage_GetId : public AquariteMessage, public Interfaces::IMessageSignalRecv<AquariteMessage_GetId>
	{
	public:
		static const uint8_t Index_RequestedDataFlag = 4;

	public:
		AquariteMessage_GetId();
		AquariteMessage_GetId(PanelDataTypes requested_panel_data);
		virtual ~AquariteMessage_GetId();

	public:
		PanelDataTypes RequestedPanelData() const;

	public:
		virtual std::string ToString() const;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override;

	private:
		PanelDataTypes m_RequestedPanelData;

	private:
		static const Factory::JandyMessageRegistration<Messages::AquariteMessage_GetId> g_AquariteMessage_GetId_Registration;
	};

}
// namespace AqualinkAutomate::Messages
