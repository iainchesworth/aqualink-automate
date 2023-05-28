#include <algorithm>
#include <format>
#include <functional>
#include <numeric>
#include <type_traits>
#include <utility>

#include <magic_enum.hpp>

#include "jandy/devices/aquarite_device.h"
#include "jandy/devices/iaq_device.h"
#include "jandy/devices/keypad_device.h"
#include "jandy/devices/onetouch_device.h"
#include "jandy/devices/pda_device.h"
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
#include "jandy/publisher/jandy_message_publisher.h"
#include "jandy/types/jandy_types.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Equipment
{
	JandyEquipment::JandyEquipment(boost::asio::io_context& io_context, Config::JandyConfig& config, ProtocolHandler& protocol_handler) :
		IEquipment(io_context, protocol_handler),
		m_IOContext(io_context),
		m_Devices(),
		m_IdentifiedDeviceIds(),
		m_MessageStats(),
		m_Config(config)
	{
		magic_enum::enum_for_each<Messages::JandyMessageIds>([this](auto id)
			{
				m_MessageStats[id] = 0;
			}
		);

		Messages::JandyMessage_Ack::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::JandyMessage_Message::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::JandyMessage_MessageLong::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::JandyMessage_Probe::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::JandyMessage_Status::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::JandyMessage_Unknown::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::AquariteMessage_GetId::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::AquariteMessage_Percent::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::AquariteMessage_PPM::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::IAQMessage_ControlReady::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::IAQMessage_MessageLong::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::IAQMessage_PageButton::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::IAQMessage_PageContinue::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::IAQMessage_PageEnd::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::IAQMessage_PageMessage::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::IAQMessage_PageStart::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::IAQMessage_Poll::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::IAQMessage_StartUp::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::IAQMessage_TableMessage::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::PDAMessage_Clear::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::PDAMessage_Highlight::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::PDAMessage_HighlightChars::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));
		Messages::PDAMessage_ShiftLines::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1));

		Messages::JandyMessage_Unknown::GetSignal()->connect(std::bind(&JandyEquipment::DisplayUnknownMessages, this, std::placeholders::_1));

		Publishers::JandyMessagePublisher::GetSignal()->connect(std::bind(&JandyEquipment::PublishEquipmentMessage, this, std::placeholders::_1));
	}

	auto JandyEquipment::IsDeviceRegistered(const Devices::JandyDeviceType& device_id)
	{
		auto const& device_it = std::find_if(m_Devices.cbegin(), m_Devices.cend(), [device_id](const std::unique_ptr<Devices::JandyDevice>& existing_device)
			{
				return (existing_device->DeviceId() == device_id);
			}
		);

		return device_it;
	}

	void JandyEquipment::IdentifyAndAddDevice(const Messages::JandyMessage& message)
	{
		if (Messages::JandyMessageIds::Probe == message.Id())
		{
			// A probe message is just the Aqualink master looking for a (potentially non-existant) device...do nothing.
		}
		else if (m_IdentifiedDeviceIds.contains(message.Destination().Raw()))
		{
			// Already have this device in the devices list...do nothing.
		}
		else
		{
			switch (message.Destination().Class())
			{
			case Devices::DeviceClasses::IAQ:
				LogInfo(Channel::Equipment, std::format("Adding new IAQ device with id: 0x{:02x}", message.Destination().Raw()));
				m_Devices.push_back(std::move(std::make_unique<Devices::IAQDevice>(m_IOContext, message.Destination().Raw(), m_Config, Devices::JandyControllerOperatingModes::MonitorOnly)));
				break;

			case Devices::DeviceClasses::OneTouch:
				LogInfo(Channel::Equipment, std::format("Adding new OneTouch device with id: 0x{:02x}", message.Destination().Raw()));
				m_Devices.push_back(std::move(std::make_unique<Devices::OneTouchDevice>(m_IOContext, message.Destination().Raw(), m_Config, Devices::JandyControllerOperatingModes::MonitorOnly)));
				break;

			case Devices::DeviceClasses::PDA:
				LogInfo(Channel::Equipment, std::format("Adding new PDA device with id: 0x{:02x}", message.Destination().Raw()));
				m_Devices.push_back(std::move(std::make_unique<Devices::PDADevice>(m_IOContext, message.Destination().Raw(), m_Config, Devices::JandyControllerOperatingModes::MonitorOnly)));
				break;

			case Devices::DeviceClasses::RS_Keypad:
				LogInfo(Channel::Equipment, std::format("Adding new RS Keypad device with id: 0x{:02x}", message.Destination().Raw()));
				m_Devices.push_back(std::move(std::make_unique<Devices::KeypadDevice>(m_IOContext, message.Destination().Raw(), m_Config, Devices::JandyControllerOperatingModes::MonitorOnly)));
				break;

			case Devices::DeviceClasses::SWG_Aquarite:
				LogInfo(Channel::Equipment, std::format("Adding new SWG device with id: 0x{:02x}", message.Destination().Raw()));
				m_Devices.push_back(std::move(std::make_unique<Devices::AquariteDevice>(m_IOContext, message.Destination().Raw())));
				break;

			default:
				LogDebug(Channel::Equipment, std::format("Device class ({}, 0x{:02x}) not supported.", magic_enum::enum_name(message.Destination().Class()), message.Destination().Raw()));
				break;
			}

			// So we've handled this device...no need to keep repeating ourselves...
			m_IdentifiedDeviceIds.insert(message.Destination().Raw());
		}

		// Capture statistics, given we are processing every message.
		m_MessageStats[message.Id()]++;

		LogTrace(Channel::Equipment, std::format("Stats: {} messages of type {} received", m_MessageStats[message.Id()], magic_enum::enum_name(message.Id())));
	}
	
	void JandyEquipment::DisplayUnknownMessages(const Messages::JandyMessage& message)
	{
		LogWarning(
			Channel::Equipment,
			std::format(
				"Unknown message received -> cannot process: destination {} (0x{:02x}), message id {} (0x{:02x}), length {} bytes, checksum 0x{:02x}",
				magic_enum::enum_name(message.Destination().Class()),
				message.Destination().Raw(),
				magic_enum::enum_name(message.Id()),
				message.RawId(),
				message.MessageLength(),
				message.ChecksumValue()
			)
		);
	}

	void JandyEquipment::PublishEquipmentMessage(const Messages::JandyMessage& message)
	{
		LogInfo(Channel::Equipment, "Publishing message from equipment");

		if (!m_ProtocolHandler.HandlePublish(message))
		{
			LogWarning(Channel::Equipment, "Failed to publish message");
		}
		else
		{
			///FIXME stats
		}
	}

	bool JandyEquipment::AddEmulatedDevice(std::unique_ptr<Devices::JandyDevice> device)
	{
		bool added_device = false;

		if (m_Devices.end() != IsDeviceRegistered(device->DeviceId()))
		{
			LogWarning(Channel::Equipment, std::format("Cannot add emulated device; id (0x{:02x}) already registered", device->DeviceId().Raw()));
		}
		else
		{
			LogInfo(Channel::Equipment, std::format("Adding new emulated device with id: 0x{:02x}", device->DeviceId().Raw()));

			m_IdentifiedDeviceIds.insert(device->DeviceId().Raw());
			m_Devices.push_back(std::move(device));
		}

		return added_device;
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
