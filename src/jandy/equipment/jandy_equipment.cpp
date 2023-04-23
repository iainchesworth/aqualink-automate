#include <algorithm>
#include <format>
#include <numeric>
#include <type_traits>
#include <utility>

#include <magic_enum.hpp>

#include "jandy/devices/aquarite_device.h"
#include "jandy/devices/iaq_device.h"
#include "jandy/devices/onetouch_device.h"
#include "jandy/equipment/jandy_equipment.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_message.h"
#include "jandy/messages/jandy_message_message_long.h"
#include "jandy/messages/jandy_message_probe.h"
#include "jandy/messages/jandy_message_status.h"
#include "jandy/messages/jandy_message_unknown.h"
#include "jandy/messages/aquarite/aquarite_message_getid.h"
#include "jandy/messages/aquarite/aquarite_message_percent.h"
#include "jandy/messages/aquarite/aquarite_message_ppm.h"
#include "jandy/messages/iaq/iaq_message_control_ready.h"
#include "jandy/messages/iaq/iaq_message_message_long.h"
#include "jandy/messages/iaq/iaq_message_page_button.h"
#include "jandy/messages/iaq/iaq_message_page_continue.h"
#include "jandy/messages/iaq/iaq_message_page_end.h"
#include "jandy/messages/iaq/iaq_message_page_message.h"
#include "jandy/messages/iaq/iaq_message_page_start.h"
#include "jandy/messages/iaq/iaq_message_poll.h"
#include "jandy/messages/iaq/iaq_message_startup.h"
#include "jandy/messages/iaq/iaq_message_table_message.h"
#include "jandy/messages/pda/pda_message_clear.h"
#include "jandy/messages/pda/pda_message_highlight.h"
#include "jandy/messages/pda/pda_message_highlight_chars.h"
#include "jandy/messages/pda/pda_message_shiftlines.h"
#include "jandy/types/jandy_types.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Equipment
{
	JandyEquipment::JandyEquipment(boost::asio::io_context& io_context, ProtocolHandler& protocol_handler) :
		IEquipment(io_context, protocol_handler),
		m_IOContext(io_context),
		m_Devices()
	{
		auto create_aquarite_device = [this](const auto& msg)
		{
			if (auto device = IsDeviceRegistered(msg.DestinationId()); m_Devices.end() == device)
			{
				auto aquarite_device = std::make_shared<Devices::AquariteDevice>(m_IOContext, msg.DestinationId());
				m_Devices.push_back(std::move(aquarite_device));
			}
		};

		Messages::AquariteMessage_GetId::GetSignal()->connect(create_aquarite_device);
		Messages::AquariteMessage_Percent::GetSignal()->connect(create_aquarite_device);
		Messages::AquariteMessage_PPM::GetSignal()->connect(create_aquarite_device);

		auto create_iaq_device = [this](const auto& msg)
		{
			if (auto device = IsDeviceRegistered(msg.DestinationId()); m_Devices.end() == device)
			{
				auto iaq_device = std::make_shared<Devices::IAQDevice>(m_IOContext, msg.DestinationId());
				m_Devices.push_back(std::move(iaq_device));
			}
		};

		Messages::IAQMessage_ControlReady::GetSignal()->connect(create_iaq_device);
		Messages::IAQMessage_MessageLong::GetSignal()->connect(create_iaq_device);
		Messages::IAQMessage_PageButton::GetSignal()->connect(create_iaq_device);
		Messages::IAQMessage_PageContinue::GetSignal()->connect(create_iaq_device);
		Messages::IAQMessage_PageEnd::GetSignal()->connect(create_iaq_device);
		Messages::IAQMessage_PageMessage::GetSignal()->connect(create_iaq_device);
		Messages::IAQMessage_PageStart::GetSignal()->connect(create_iaq_device);
		Messages::IAQMessage_Poll::GetSignal()->connect(create_iaq_device);
		Messages::IAQMessage_StartUp::GetSignal()->connect(create_iaq_device);
		Messages::IAQMessage_TableMessage::GetSignal()->connect(create_iaq_device);

		auto create_onetouch_device = [this](const auto& msg)
		{
			if (auto device = IsDeviceRegistered(msg.DestinationId()); m_Devices.end() == device)
			{
				auto onetouch_device = std::make_shared<Devices::OneTouchDevice>(m_IOContext, msg.DestinationId());
				m_Devices.push_back(std::move(onetouch_device));
			}
		};

		Messages::JandyMessage_MessageLong::GetSignal()->connect(create_onetouch_device);
		Messages::PDAMessage_Clear::GetSignal()->connect(create_onetouch_device);
		Messages::PDAMessage_Highlight::GetSignal()->connect(create_onetouch_device);
		Messages::PDAMessage_HighlightChars::GetSignal()->connect(create_onetouch_device);
		Messages::PDAMessage_ShiftLines::GetSignal()->connect(create_onetouch_device);

		magic_enum::enum_for_each<Messages::JandyMessageIds>([this](auto id)
			{
				m_MessageStats[id] = 0;
			}
		);

		auto message_statistics_capture = [this](const auto& msg)
		{
			m_MessageStats[msg.MessageId()]++;

			LogTrace(Channel::Equipment, std::format("Stats: {} messages of type {} received", m_MessageStats[msg.MessageId()], magic_enum::enum_name(msg.MessageId())));
		};

		Messages::JandyMessage_Ack::GetSignal()->connect(message_statistics_capture);
		Messages::JandyMessage_Message::GetSignal()->connect(message_statistics_capture);
		Messages::JandyMessage_MessageLong::GetSignal()->connect(message_statistics_capture);
		Messages::JandyMessage_Probe::GetSignal()->connect(message_statistics_capture);
		Messages::JandyMessage_Status::GetSignal()->connect(message_statistics_capture);
		Messages::JandyMessage_Unknown::GetSignal()->connect(message_statistics_capture);
		
		Messages::AquariteMessage_GetId::GetSignal()->connect(message_statistics_capture);
		Messages::AquariteMessage_Percent::GetSignal()->connect(message_statistics_capture);
		Messages::AquariteMessage_PPM::GetSignal()->connect(message_statistics_capture);

		Messages::IAQMessage_ControlReady::GetSignal()->connect(message_statistics_capture);
		Messages::IAQMessage_MessageLong::GetSignal()->connect(message_statistics_capture);
		Messages::IAQMessage_PageButton::GetSignal()->connect(message_statistics_capture);
		Messages::IAQMessage_PageContinue::GetSignal()->connect(message_statistics_capture);
		Messages::IAQMessage_PageEnd::GetSignal()->connect(message_statistics_capture);
		Messages::IAQMessage_PageMessage::GetSignal()->connect(message_statistics_capture);
		Messages::IAQMessage_PageStart::GetSignal()->connect(message_statistics_capture);
		Messages::IAQMessage_Poll::GetSignal()->connect(message_statistics_capture);
		Messages::IAQMessage_StartUp::GetSignal()->connect(message_statistics_capture);
		Messages::IAQMessage_TableMessage::GetSignal()->connect(message_statistics_capture);
	}

	auto JandyEquipment::IsDeviceRegistered(Interfaces::IDevice::DeviceId device_id)
	{
		auto device_it = std::find_if(m_Devices.cbegin(), m_Devices.cend(), [device_id](const std::shared_ptr<Interfaces::IDevice> existing_device)
			{
				return (existing_device->Id() == device_id);
			}
		);

		return device_it;
	}

	void JandyEquipment::StopAndCleanUp()
	{
		magic_enum::enum_for_each<Messages::JandyMessageIds>([this](Messages::JandyMessageIds id)
			{
				LogInfo(Channel::Devices, std::format("Stats: processed {} messages of type {}", m_MessageStats[id], magic_enum::enum_name(id)));
				LogInfo(Channel::Equipment, std::format("Stats: {} total messages received", std::accumulate(m_MessageStats.cbegin(), m_MessageStats.cend(), static_cast<uint64_t>(0), [](const uint64_t previous, const decltype(m_MessageStats)::value_type& elem)
					{
						return previous + elem.second;
					})
				));
			}
		);
	}
}
// namespace AqualinkAutomate::Equipment
