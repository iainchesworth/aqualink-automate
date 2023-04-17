#include <algorithm>
#include <format>
#include <utility>

#include <boost/bind/bind.hpp>

#include "jandy/devices/aquarite_device.h"
#include "jandy/equipment/jandy_equipment.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Equipment
{
	JandyEquipment::JandyEquipment(boost::asio::io_context& io_context, ProtocolHandler& protocol_handler, MessageBridge& message_bridge) :
		IEquipment(io_context, protocol_handler, message_bridge),
		m_IOContext(io_context),
		m_Devices()
	{
		m_MessageBridge.Signal_AllJandyMessages.connect(boost::bind(&JandyEquipment::Slot_AllMessageTypes, this, boost::placeholders::_1));
		m_MessageBridge.Signal_Aquarite_GetId.connect(boost::bind(&JandyEquipment::Slot_Aquarite_GetId, this, boost::placeholders::_1));
		m_MessageBridge.Signal_Aquarite_Percent.connect(boost::bind(&JandyEquipment::Slot_Aquarite_Percent, this, boost::placeholders::_1));
		m_MessageBridge.Signal_Aquarite_PPM.connect(boost::bind(&JandyEquipment::Slot_Aquarite_PPM, this, boost::placeholders::_1));
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

	void JandyEquipment::Slot_AllMessageTypes(const Types::JandyMessageTypePtr& msg)
	{
		m_MessagesProcessed++;

		LogDebug(Channel::Equipment, std::format("STUFF SIGNAL -> SLOT....TOTAL MESSAGES: {}", m_MessagesProcessed));
	}

	void JandyEquipment::Slot_Aquarite_GetId(const std::shared_ptr<Messages::AquariteMessage_GetId>& msg)
	{
		LogDebug(Channel::Equipment, "Jandy Equipment received a AquariteMessage_GetId signal.");
	}

	void JandyEquipment::Slot_Aquarite_Percent(const std::shared_ptr<Messages::AquariteMessage_Percent>& msg)
	{
		LogDebug(Channel::Equipment, "Jandy Equipment received a AquariteMessage_Percent signal.");

		if (nullptr == msg)
		{
			LogDebug(Channel::Equipment, "Received an invalid AquariteMessage_Percent message...received a nullptr");
		}
		else if (auto device = IsDeviceRegistered(msg->DestinationId()); m_Devices.end() == device)
		{
			auto aquarite_device = std::make_shared<Devices::AquariteDevice>(m_IOContext, msg->DestinationId());
			aquarite_device->RequestedGeneratingLevel(msg->GeneratingPercentage());

			m_Devices.push_back(std::move(aquarite_device));
		}
		else if (auto aquarite_device = std::dynamic_pointer_cast<Devices::AquariteDevice>(*device); nullptr == aquarite_device)
		{
			LogDebug(Channel::Equipment, std::format("Failed to convert device to an Aquarite device; original device destination id -> {}", (*device)->Id()));
		}
		else
		{
			LogDebug(Channel::Equipment, std::format("Aquarite Device: received new requested generating level -> {}%", msg->GeneratingPercentage()));
			aquarite_device->RequestedGeneratingLevel(msg->GeneratingPercentage());
		}
	}

	void JandyEquipment::Slot_Aquarite_PPM(const std::shared_ptr<Messages::AquariteMessage_PPM>& msg)
	{
		LogDebug(Channel::Equipment, "Jandy Equipment received a AquariteMessage_PPM signal.");

		if (nullptr == msg)
		{
			LogDebug(Channel::Equipment, "Received an invalid AquariteMessage_PPM message...received a nullptr");
		}
		else if (auto device = IsDeviceRegistered(msg->DestinationId()); m_Devices.end() == device)
		{
			auto aquarite_device = std::make_shared<Devices::AquariteDevice>(m_IOContext, msg->DestinationId());
			aquarite_device->ReportedGeneratingLevel(msg->GeneratingPercentage());
			aquarite_device->ReportedGeneratingLevel(msg->SaltConcentrationPPM());

			m_Devices.push_back(std::move(aquarite_device));
		}
		else if (auto aquarite_device = std::dynamic_pointer_cast<Devices::AquariteDevice>(*device); nullptr == aquarite_device)
		{
			LogDebug(Channel::Equipment, std::format("Failed to convert device to an Aquarite device; original device destination id -> {}", (*device)->Id()));
		}
		else
		{
			LogDebug(Channel::Equipment, std::format("Aquarite Device: received new reported generating level and salt concentration -> {}% and {} PPM", msg->GeneratingPercentage(), msg->SaltConcentrationPPM()));
			aquarite_device->ReportedGeneratingLevel(msg->GeneratingPercentage());
			aquarite_device->ReportedGeneratingLevel(msg->SaltConcentrationPPM());
		}
	}
}
// namespace AqualinkAutomate::Equipment
