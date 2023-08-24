#include <algorithm>
#include <format>
#include <functional>
#include <numeric>
#include <type_traits>
#include <utility>

#include <magic_enum.hpp>
#include <magic_enum_utility.hpp>

#include "jandy/devices/aquarite_device.h"
#include "jandy/devices/iaq_device.h"
#include "jandy/devices/keypad_device.h"
#include "jandy/devices/onetouch_device.h"
#include "jandy/devices/pda_device.h"
#include "jandy/devices/serial_adapter_device.h"
#include "jandy/equipment/jandy_equipment.h"
#include "jandy/formatters/jandy_device_formatters.h"
#include "jandy/formatters/stats_counter_formatter.h"
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
#include "jandy/messages/serial_adapter/serial_adapter_message_dev_ready.h"
#include "jandy/messages/serial_adapter/serial_adapter_message_dev_status.h"
#include "jandy/types/jandy_types.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Equipment
{
	JandyEquipment::JandyEquipment(boost::asio::io_context& io_context, Kernel::HubLocator& hub_locator) :
		IEquipment(io_context),
		m_IdentifiedDeviceIds(),
		m_MessageConnections(),
		m_HubLocator(hub_locator)
	{
		m_DataHub = m_HubLocator.Find<Kernel::DataHub>();
		m_StatsHub = m_HubLocator.Find<Kernel::StatisticsHub>();

		magic_enum::enum_for_each<Messages::JandyMessageIds>([this](auto id)
			{
				m_StatsHub->MessageCounts[id] = 0;
			}
		);

		m_MessageConnections.push_back(Messages::JandyMessage_Ack::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::JandyMessage_Message::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::JandyMessage_MessageLong::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::JandyMessage_Probe::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::JandyMessage_Status::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::JandyMessage_Unknown::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::AquariteMessage_GetId::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::AquariteMessage_Percent::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::AquariteMessage_PPM::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::IAQMessage_ControlReady::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::IAQMessage_MessageLong::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::IAQMessage_PageButton::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::IAQMessage_PageContinue::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::IAQMessage_PageEnd::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::IAQMessage_PageMessage::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::IAQMessage_PageStart::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::IAQMessage_Poll::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::IAQMessage_StartUp::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::IAQMessage_TableMessage::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::PDAMessage_Clear::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::PDAMessage_Highlight::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::PDAMessage_HighlightChars::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::PDAMessage_ShiftLines::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::SerialAdapterMessage_DevReady::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));
		m_MessageConnections.push_back(Messages::SerialAdapterMessage_DevStatus::GetSignal()->connect(std::bind(&JandyEquipment::IdentifyAndAddDevice, this, std::placeholders::_1)));

		m_MessageConnections.push_back(Messages::JandyMessage_Unknown::GetSignal()->connect(std::bind(&JandyEquipment::DisplayUnknownMessages, this, std::placeholders::_1)));
	}

	JandyEquipment::~JandyEquipment()
	{
		for (auto& connection : m_MessageConnections)
		{
			connection.disconnect();
		}
		
		magic_enum::enum_for_each<Messages::JandyMessageIds>([this](Messages::JandyMessageIds id)
			{
				LogInfo(Channel::Devices, std::format("Stats: processed {} messages of type {}", m_StatsHub->MessageCounts[id], magic_enum::enum_name(id)));
			}
		);

		LogInfo(
			Channel::Equipment, 
			std::format("Stats: {} total messages received", 
				std::accumulate(m_StatsHub->MessageCounts.cbegin(), m_StatsHub->MessageCounts.cend(), static_cast<uint64_t>(0), [](const uint64_t previous, const decltype(m_StatsHub->MessageCounts)::value_type& elem)
				{
					return previous + elem.second.Count();
				})
			)
		);
	}

	bool JandyEquipment::AddEmulatedDevice(std::unique_ptr<Devices::JandyDevice> device)
	{
		bool added_device = false;

		if (IsDeviceRegistered(device->DeviceId()))
		{
			LogWarning(Channel::Equipment, std::format("Cannot add emulated device; id ({}) already registered", device->DeviceId().Id()));
		}
		else
		{
			LogInfo(Channel::Equipment, std::format("Adding new emulated device with id: {}", device->DeviceId().Id()));

			m_IdentifiedDeviceIds.insert(device->DeviceId().Id());
			IEquipment::AddDevice(std::move(device));
		}

		return added_device;
	}

	void JandyEquipment::IdentifyAndAddDevice(const Messages::JandyMessage& message)
	{
		if (Messages::JandyMessageIds::Probe == message.Id())
		{
			// A probe message is just the Aqualink master looking for a (potentially non-existant) device...do nothing.
		}
		else if (m_IdentifiedDeviceIds.contains(message.Destination().Id()))
		{
			// Already have this device in the devices list...do nothing.
		}
		else
		{
			auto device_id = std::make_unique<Devices::JandyDeviceType>(message.Destination().Id());

			switch (message.Destination().Class())
			{
			case Devices::DeviceClasses::IAQ:
				LogInfo(Channel::Equipment, std::format("Adding new IAQ device with id: {}", message.Destination().Id()));
				IEquipment::AddDevice(std::move(std::make_unique<Devices::IAQDevice>(m_IOContext, std::move(device_id), m_HubLocator, false)));
				break;

			case Devices::DeviceClasses::OneTouch:
				LogInfo(Channel::Equipment, std::format("Adding new OneTouch device with id: {}", message.Destination().Id()));
				IEquipment::AddDevice(std::move(std::make_unique<Devices::OneTouchDevice>(m_IOContext, std::move(device_id), m_HubLocator, false)));
				break;

			case Devices::DeviceClasses::PDA:
				LogInfo(Channel::Equipment, std::format("Adding new PDA device with id: {}", message.Destination().Id()));
				IEquipment::AddDevice(std::move(std::make_unique<Devices::PDADevice>(m_IOContext, std::move(device_id), m_HubLocator, false)));
				break;

			case Devices::DeviceClasses::RS_Keypad:
				LogInfo(Channel::Equipment, std::format("Adding new RS Keypad device with id: {}", message.Destination().Id()));
				IEquipment::AddDevice(std::move(std::make_unique<Devices::KeypadDevice>(m_IOContext, std::move(device_id), m_HubLocator, false)));
				break;

			case Devices::DeviceClasses::SerialAdapter:
				LogInfo(Channel::Equipment, std::format("Adding new Serial Adapter device with id: {}", message.Destination().Id()));
				IEquipment::AddDevice(std::move(std::make_unique<Devices::SerialAdapterDevice>(m_IOContext, std::move(device_id), m_HubLocator, false)));
				break;

			case Devices::DeviceClasses::SWG_Aquarite:
				LogInfo(Channel::Equipment, std::format("Adding new SWG device with id: {}", message.Destination().Id()));
				IEquipment::AddDevice(std::move(std::make_unique<Devices::AquariteDevice>(m_IOContext, std::move(device_id))));
				break;

			default:
				LogDebug(Channel::Equipment, std::format("Device class ({}, {}) not supported.", magic_enum::enum_name(message.Destination().Class()), message.Destination().Id()));
				break;
			}

			// So we've handled this device...no need to keep repeating ourselves...
			m_IdentifiedDeviceIds.insert(message.Destination().Id());
		}

		// Capture statistics, given we are processing every message.
		m_StatsHub->MessageCounts[message.Id()]++;

		LogTrace(Channel::Equipment, std::format("Stats: {} messages of type {} received", m_StatsHub->MessageCounts[message.Id()], magic_enum::enum_name(message.Id())));
	}
	
	void JandyEquipment::DisplayUnknownMessages(const Messages::JandyMessage& message)
	{
		LogWarning(
			Channel::Equipment,
			std::format(
				"Unknown message received -> cannot process: destination {} ({}), message id {} (0x{:02x}), length {} bytes, checksum 0x{:02x}",
				magic_enum::enum_name(message.Destination().Class()),
				message.Destination().Id(),
				magic_enum::enum_name(message.Id()),
				message.RawId(),
				message.MessageLength(),
				message.ChecksumValue()
			)
		);
	}

}
// namespace AqualinkAutomate::Equipment
