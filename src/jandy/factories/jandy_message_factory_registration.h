#pragma once

#include <array>
#include <format>
#include <memory>

#include "concepts/jandy_message_type.h"
#include "factories/jandy_message_factory.h"
#include "messages/jandy_message_ack.h"
#include "messages/jandy_message_ids.h"
#include "messages/jandy_message_message.h"
#include "messages/jandy_message_message_long.h"
#include "messages/jandy_message_display_update.h"
#include "messages/jandy_message_probe.h"
#include "messages/jandy_message_status.h"
#include "messages/jandy_message_unknown.h"
#include "messages/chemlink/chemlink_message_response.h"
#include "messages/epump/epump_message_status.h"
#include "messages/epump/epump_message_rpm.h"
#include "messages/epump/epump_message_watts.h"
#include "messages/heater/heater_message_request.h"
#include "messages/heater/heater_message_status.h"
#include "messages/light/light_message_status.h"
#include "messages/aquarite/aquarite_message_getid.h"
#include "messages/aquarite/aquarite_message_percent.h"
#include "messages/aquarite/aquarite_message_setpercent.h"
#include "messages/aquarite/aquarite_message_ppm.h"
#include "messages/iaq/iaq_message_aux_status.h"
#include "messages/iaq/iaq_message_command_ready.h"
#include "messages/iaq/iaq_message_control_ready.h"
#include "messages/iaq/iaq_message_device_id.h"
#include "messages/iaq/iaq_message_heartbeat.h"
#include "messages/iaq/iaq_message_main_status.h"
#include "messages/iaq/iaq_message_message_long.h"
#include "messages/iaq/iaq_message_onetouch_status.h"
#include "messages/iaq/iaq_message_page_button.h"
#include "messages/iaq/iaq_message_page_continue.h"
#include "messages/iaq/iaq_message_page_end.h"
#include "messages/iaq/iaq_message_page_message.h"
#include "messages/iaq/iaq_message_page_start.h"
#include "messages/iaq/iaq_message_poll.h"
#include "messages/iaq/iaq_message_startup.h"
#include "messages/iaq/iaq_message_table_message.h"
#include "messages/iaq/iaq_message_title_message.h"
#include "messages/pda/pda_message_clear.h"
#include "messages/pda/pda_message_highlight.h"
#include "messages/pda/pda_message_highlight_chars.h"
#include "messages/pda/pda_message_shiftlines.h"
#include "messages/serial_adapter/serial_adapter_message_dev_ready.h"
#include "messages/serial_adapter/serial_adapter_message_dev_status.h"
#include "types/jandy_types.h"

namespace AqualinkAutomate::Factory
{

#define REGISTER_HOT_MESSAGE(MessageType, MessageId) \
    Factory::MessageRegistration<MessageType, MessageId, true>

#define REGISTER_MESSAGE(MessageType, MessageId) \
    Factory::MessageRegistration<MessageType, MessageId, false>

	using JandyMessageFactoryT = Factory::JandyMessageFactory<
		// Register the core Jandy message types
		REGISTER_HOT_MESSAGE(Messages::JandyMessage_Ack, Messages::JandyMessageIds::Ack),
		REGISTER_HOT_MESSAGE(Messages::JandyMessage_Status, Messages::JandyMessageIds::Status),
		REGISTER_HOT_MESSAGE(Messages::JandyMessage_Probe, Messages::JandyMessageIds::Probe),
		REGISTER_MESSAGE(Messages::JandyMessage_Message, Messages::JandyMessageIds::Message),
		REGISTER_MESSAGE(Messages::JandyMessage_MessageLong, Messages::JandyMessageIds::MessageLong),

		// Register the display protocol message types
		REGISTER_MESSAGE(Messages::JandyMessage_DisplayUpdate, Messages::JandyMessageIds::DisplayUpdate),

		// Register the UNKNOWN type for all "seen-but-unknown" message types
		REGISTER_MESSAGE(Messages::JandyMessage_Unknown, Messages::JandyMessageIds::Unknown_PDA_1B),
		REGISTER_MESSAGE(Messages::JandyMessage_Unknown, Messages::JandyMessageIds::Unknown_ReadyControl),
		REGISTER_MESSAGE(Messages::JandyMessage_Unknown, Messages::JandyMessageIds::IAQ_PageSubMessage),
		REGISTER_MESSAGE(Messages::JandyMessage_Unknown, Messages::JandyMessageIds::Unknown),

		// Register the ePump message types
		REGISTER_MESSAGE(Messages::EPumpMessage_Status, Messages::JandyMessageIds::EPUMP_Status),
		REGISTER_MESSAGE(Messages::EPumpMessage_RPM, Messages::JandyMessageIds::EPUMP_RPM),
		REGISTER_MESSAGE(Messages::EPumpMessage_Watts, Messages::JandyMessageIds::EPUMP_Watts),

		// Register the Chemlink message types
		REGISTER_MESSAGE(Messages::ChemlinkMessage_Response, Messages::JandyMessageIds::Chemlink_Response),

		// Register the Heater message types
		REGISTER_MESSAGE(Messages::HeaterMessage_Request, Messages::JandyMessageIds::Heater_Request),
		REGISTER_MESSAGE(Messages::HeaterMessage_Status, Messages::JandyMessageIds::Heater_Status),

		// Register the Jandy Light message types
		REGISTER_MESSAGE(Messages::LightMessage_Status, Messages::JandyMessageIds::Light_Status),

		// Register the Aquarite message types
		REGISTER_MESSAGE(Messages::AquariteMessage_GetId, Messages::JandyMessageIds::AQUARITE_GetId),
		REGISTER_MESSAGE(Messages::AquariteMessage_Percent, Messages::JandyMessageIds::AQUARITE_Percent),
		REGISTER_MESSAGE(Messages::AquariteMessage_SetPercent, Messages::JandyMessageIds::AQUARITE_SetPercent),
		REGISTER_MESSAGE(Messages::AquariteMessage_PPM, Messages::JandyMessageIds::AQUARITE_PPM),

		// Register the IAQ message types
		REGISTER_MESSAGE(Messages::IAQMessage_ControlReady, Messages::JandyMessageIds::IAQ_ControlReady),
		REGISTER_MESSAGE(Messages::IAQMessage_Heartbeat, Messages::JandyMessageIds::IAQ_Heartbeat),
		REGISTER_MESSAGE(Messages::IAQMessage_DeviceId, Messages::JandyMessageIds::IAQ_DeviceId),
		REGISTER_MESSAGE(Messages::IAQMessage_MessageLong, Messages::JandyMessageIds::IAQ_MessageLong),
		REGISTER_MESSAGE(Messages::IAQMessage_PageButton, Messages::JandyMessageIds::IAQ_PageButton),
		REGISTER_MESSAGE(Messages::IAQMessage_PageContinue, Messages::JandyMessageIds::IAQ_PageContinue),
		REGISTER_MESSAGE(Messages::IAQMessage_PageEnd, Messages::JandyMessageIds::IAQ_PageEnd),
		REGISTER_MESSAGE(Messages::IAQMessage_PageMessage, Messages::JandyMessageIds::IAQ_PageMessage),
		REGISTER_MESSAGE(Messages::IAQMessage_PageStart, Messages::JandyMessageIds::IAQ_PageStart),
		REGISTER_MESSAGE(Messages::IAQMessage_Poll, Messages::JandyMessageIds::IAQ_Poll),
		REGISTER_MESSAGE(Messages::IAQMessage_StartUp, Messages::JandyMessageIds::IAQ_StartUp),
		REGISTER_MESSAGE(Messages::IAQMessage_TableMessage, Messages::JandyMessageIds::IAQ_TableMessage),
		REGISTER_MESSAGE(Messages::IAQMessage_TitleMessage, Messages::JandyMessageIds::IAQ_TitleMessage),
		REGISTER_MESSAGE(Messages::IAQMessage_MainStatus, Messages::JandyMessageIds::IAQ_MainStatus),
		REGISTER_MESSAGE(Messages::IAQMessage_OneTouchStatus, Messages::JandyMessageIds::IAQ_OneTouchStatus),
		REGISTER_MESSAGE(Messages::IAQMessage_AuxStatus, Messages::JandyMessageIds::IAQ_AuxStatus),
		REGISTER_MESSAGE(Messages::IAQMessage_CommandReady, Messages::JandyMessageIds::IAQ_CommandReady),

		// Register the PDA message types
		REGISTER_MESSAGE(Messages::PDAMessage_Clear, Messages::JandyMessageIds::PDA_Clear),
		REGISTER_MESSAGE(Messages::PDAMessage_Highlight, Messages::JandyMessageIds::PDA_Highlight),
		REGISTER_MESSAGE(Messages::PDAMessage_HighlightChars, Messages::JandyMessageIds::PDA_HighlightChars),
		REGISTER_MESSAGE(Messages::PDAMessage_ShiftLines, Messages::JandyMessageIds::PDA_ShiftLines),

		// Register the RSSA Serial Adapter message types
		REGISTER_MESSAGE(Messages::SerialAdapterMessage_DevReady, Messages::JandyMessageIds::RSSA_DevReady),
		REGISTER_MESSAGE(Messages::SerialAdapterMessage_DevStatus, Messages::JandyMessageIds::RSSA_DevStatus)
	>;

}
// namespace AqualinkAutomate::Factory
