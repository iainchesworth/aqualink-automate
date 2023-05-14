#pragma once

#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "interfaces/imessagesignal_send.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/aquarite/aquarite_message.h"
#include "jandy/publisher/jandy_message_publisher.h"

namespace AqualinkAutomate::Messages
{

	enum class PanelDataTypes : uint8_t
	{
		PanelRevision = 0x01,
		PanelType = 0x02,
		Unknown = 0xFF
	};

	class AquariteMessage_GetId : public AquariteMessage, public Interfaces::IMessageSignalRecv<AquariteMessage_GetId>, public Interfaces::IMessageSignalSend<AquariteMessage_GetId, Publishers::JandyMessagePublisher>
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
		virtual void Serialize(std::vector<uint8_t>& message_bytes) const;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes);

	private:
		PanelDataTypes m_RequestedPanelData;

	private:
		static const Factory::JandyMessageRegistration<Messages::AquariteMessage_GetId> g_AquariteMessage_GetId_Registration;
	};

}
// namespace AqualinkAutomate::Messages
