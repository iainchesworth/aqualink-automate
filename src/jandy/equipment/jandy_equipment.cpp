#include <algorithm>
#include <format>
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
#include "jandy/types/jandy_types.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Equipment
{
	JandyEquipment::JandyEquipment(boost::asio::io_context& io_context, ProtocolHandler& protocol_handler) :
		IEquipment(io_context, protocol_handler),
		m_IOContext(io_context),
		m_Devices(),
		m_IdentifiedDeviceIds(),
		m_MessageStats()
	{
		magic_enum::enum_for_each<Messages::JandyMessageIds>([this](auto id)
			{
				m_MessageStats[id] = 0;
			}
		);

		auto identify_and_add_device = [this](const auto& msg)
		{
			if (Messages::JandyMessageIds::Probe == msg.Id())
			{
				// A probe message is just the Aqualink master looking for a (potentially non-existant) device...do nothing.
			}
			else if (m_IdentifiedDeviceIds.contains(msg.Destination().Raw()))
			{
				// Already have this device in the devices list...do nothing.
			}
			else 
			{
				switch (msg.Destination().Class())
				{
				case Devices::DeviceClasses::IAQ:
					LogInfo(Channel::Equipment, std::format("Adding new IAQ device with id: 0x{:02x}", msg.Destination().Raw()));
					m_Devices.push_back(std::move(std::make_unique<Devices::IAQDevice>(m_IOContext, msg.Destination().Raw())));
					break;

				case Devices::DeviceClasses::OneTouch:
					LogInfo(Channel::Equipment, std::format("Adding new OneTouch device with id: 0x{:02x}", msg.Destination().Raw()));
					m_Devices.push_back(std::move(std::make_unique<Devices::OneTouchDevice>(m_IOContext, msg.Destination().Raw())));
					break;

				case Devices::DeviceClasses::PDA:
					LogInfo(Channel::Equipment, std::format("Adding new PDA device with id: 0x{:02x}", msg.Destination().Raw()));
					m_Devices.push_back(std::move(std::make_unique<Devices::PDADevice>(m_IOContext, msg.Destination().Raw())));
					break;

				case Devices::DeviceClasses::RS_Keypad:
					LogInfo(Channel::Equipment, std::format("Adding new RS Keypad device with id: 0x{:02x}", msg.Destination().Raw()));
					m_Devices.push_back(std::move(std::make_unique<Devices::KeypadDevice>(m_IOContext, msg.Destination().Raw())));
					break;

				case Devices::DeviceClasses::SWG_Aquarite:
					LogInfo(Channel::Equipment, std::format("Adding new SWG device with id: 0x{:02x}", msg.Destination().Raw()));
					m_Devices.push_back(std::move(std::make_unique<Devices::AquariteDevice>(m_IOContext, msg.Destination().Raw())));
					break;

				default:
					LogDebug(Channel::Equipment, std::format("Device class ({}, 0x{:02x}) not supported.", magic_enum::enum_name(msg.Destination().Class()), msg.Destination().Raw()));
					break;
				}

				// So we've handled this device...no need to keep repeating ourselves...
				m_IdentifiedDeviceIds.insert(msg.Destination().Raw());
			}

			// Capture statistics, given we are processing every message.
			m_MessageStats[msg.Id()]++;

			LogTrace(Channel::Equipment, std::format("Stats: {} messages of type {} received", m_MessageStats[msg.Id()], magic_enum::enum_name(msg.Id())));
		};

		Messages::JandyMessage_Ack::GetSignal()->connect(identify_and_add_device);
		Messages::JandyMessage_Message::GetSignal()->connect(identify_and_add_device);
		Messages::JandyMessage_MessageLong::GetSignal()->connect(identify_and_add_device);
		Messages::JandyMessage_Probe::GetSignal()->connect(identify_and_add_device);
		Messages::JandyMessage_Status::GetSignal()->connect(identify_and_add_device);
		Messages::JandyMessage_Unknown::GetSignal()->connect(identify_and_add_device);		
		Messages::AquariteMessage_GetId::GetSignal()->connect(identify_and_add_device);
		Messages::AquariteMessage_Percent::GetSignal()->connect(identify_and_add_device);
		Messages::AquariteMessage_PPM::GetSignal()->connect(identify_and_add_device);
		Messages::IAQMessage_ControlReady::GetSignal()->connect(identify_and_add_device);
		Messages::IAQMessage_MessageLong::GetSignal()->connect(identify_and_add_device);
		Messages::IAQMessage_PageButton::GetSignal()->connect(identify_and_add_device);
		Messages::IAQMessage_PageContinue::GetSignal()->connect(identify_and_add_device);
		Messages::IAQMessage_PageEnd::GetSignal()->connect(identify_and_add_device);
		Messages::IAQMessage_PageMessage::GetSignal()->connect(identify_and_add_device);
		Messages::IAQMessage_PageStart::GetSignal()->connect(identify_and_add_device);
		Messages::IAQMessage_Poll::GetSignal()->connect(identify_and_add_device);
		Messages::IAQMessage_StartUp::GetSignal()->connect(identify_and_add_device);
		Messages::IAQMessage_TableMessage::GetSignal()->connect(identify_and_add_device);
		Messages::PDAMessage_Clear::GetSignal()->connect(identify_and_add_device);
		Messages::PDAMessage_Highlight::GetSignal()->connect(identify_and_add_device);
		Messages::PDAMessage_HighlightChars::GetSignal()->connect(identify_and_add_device);
		Messages::PDAMessage_ShiftLines::GetSignal()->connect(identify_and_add_device);

		auto display_unknown_messages = [this](const auto& msg)
		{
			LogWarning(
				Channel::Equipment, 
				std::format(
					"Unknown message received -> cannot process: destination {} (0x{:02x}), message id {} (0x{:02x}), length {} bytes, checksum 0x{:02x}", 
					magic_enum::enum_name(msg.Destination().Class()),
					msg.Destination().Raw(),
					magic_enum::enum_name(msg.Id()),
					msg.RawId(),
					msg.MessageLength(),
					msg.ChecksumValue()
				)
			);
		};

		Messages::JandyMessage_Unknown::GetSignal()->connect(display_unknown_messages);
	}

	auto JandyEquipment::IsDeviceRegistered(Interfaces::IDevice::DeviceId device_id)
	{
		auto const& device_it = std::find_if(m_Devices.cbegin(), m_Devices.cend(), [device_id](const std::unique_ptr<Interfaces::IDevice>& existing_device)
			{
				return (existing_device->Id() == device_id);
			}
		);

		return device_it;
	}

	bool JandyEquipment::AddEmulatedDevice(std::unique_ptr<Interfaces::IDevice> device)
	{
		bool added_device = false;

		if (m_Devices.end() != IsDeviceRegistered(device->Id()))
		{
			LogWarning(Channel::Equipment, std::format("Cannot add emulated device; id (0x{:02x}) already registered", device->Id()));
		}
		else
		{
			LogInfo(Channel::Equipment, std::format("Adding new emulated device with id: 0x{:02x}", device->Id()));

			m_IdentifiedDeviceIds.insert(device->Id());
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
