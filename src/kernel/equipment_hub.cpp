#include <algorithm>

#include "kernel/equipment_hub.h"

namespace AqualinkAutomate::Kernel
{

	EquipmentHub::EquipmentHub() :
		IHub()
	{
	}

	EquipmentHub::~EquipmentHub()
	{
	}

	const std::vector<std::shared_ptr<Interfaces::IEquipment>>& EquipmentHub::ActiveEquipment() const
	{
		return m_ActiveEquipment;
	}

	const std::vector<std::shared_ptr<Interfaces::IDevice>>& EquipmentHub::ActiveDevices() const
	{
		return m_ActiveDevices;
	}

	void EquipmentHub::AddEquipment(std::shared_ptr<Interfaces::IEquipment> equipment)
	{
		if (IsEquipmentRegistered(equipment))
		{
			// Do nothing...this equipment is already being tracked.
			LogDebug(Channel::Equipment, "Equipment has already been registered in the equipment hub...ignoring duplicate registration");
		}
		else
		{
			m_ActiveEquipment.push_back(equipment);
			CheckAndRegisterForUpdateEvents(equipment);
		}
	}

	void EquipmentHub::AddDevice(std::shared_ptr<Interfaces::IDevice> device)
	{
		if (IsDeviceRegistered(device->DeviceId()))
		{
			// Do nothing...this device is already being tracked.
			LogDebug(Channel::Devices, "Device has already been registered in the equipment hub...ignoring duplicate registration");
		}
		else
		{
			m_ActiveDevices.push_back(device);
			CheckAndRegisterForUpdateEvents(device);
		}
	}

	bool EquipmentHub::IsEquipmentRegistered(const std::shared_ptr<Interfaces::IEquipment> equipment) const
	{
		bool equipment_exists = std::any_of
		(
			m_ActiveEquipment.begin(), 
			m_ActiveEquipment.end(),
			[&equipment](const auto& equipment_ptr)
			{
				return equipment_ptr == equipment;
			}
		);

		return equipment_exists;
	}

	bool EquipmentHub::IsDeviceRegistered(const Interfaces::IDeviceIdentifier& device_id) const
	{
		bool device_exists = std::any_of
		(
			m_ActiveDevices.begin(),
			m_ActiveDevices.end(),
			[&device_id](const auto& device_ptr)
			{
				return (device_id == device_ptr->DeviceId());
			}
		);

		return device_exists;
	}

}
// namespace AqualinkAutomate::Equipment
