#include <algorithm>
#include <format>
#include <type_traits>
#include <utility>

#include "jandy/devices/aquarite_device.h"
#include "jandy/devices/iaq_device.h"
#include "jandy/equipment/jandy_equipment.h"
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

		auto create_iaq_device = [this](const auto & msg)
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

		auto message_statistics_capture = [this](const auto& msg)
		{
			LogDebug(Channel::Equipment, std::format("STUFF SIGNAL -> SLOT....TOTAL MESSAGES: {}", m_MessagesProcessed));
			m_MessagesProcessed++;
		};

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
	}
}
// namespace AqualinkAutomate::Equipment
