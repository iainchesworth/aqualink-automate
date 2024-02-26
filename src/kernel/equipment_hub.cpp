#include <algorithm>
#include <execution>
#include <typeinfo>

#include "kernel/equipment_hub.h"

namespace AqualinkAutomate::Kernel
{

	bool EquipmentHub::AddEquipment(std::unique_ptr<Interfaces::IEquipment> equipment_to_add)
	{
		if (!equipment_to_add)
		{
			LogWarning(Channel::Devices, "Cannot register equipment with equipment hub; equipment object was invalid");
		}
		else
		{
			std::type_index equipment_id_to_add = typeid(decltype(equipment_to_add)::element_type);

			if (bool equipment_exists = m_ActiveEquipment.contains(equipment_id_to_add); equipment_exists)
			{
				LogDebug(Channel::Devices, "Failed to register equipment with equipment hub; equipment id already registered");
			}
			else if (auto [_, was_inserted] = m_ActiveEquipment.emplace(std::move(equipment_id_to_add), std::move(equipment_to_add)); !was_inserted)
			{
				LogDebug(Channel::Devices, "Failed to add equipment to equipment hub; internal error while adding equipment object");
			}
			else
			{
				return true;
			}
		}

		return false;
	}

	bool EquipmentHub::AddDevice(std::unique_ptr<Interfaces::IDevice> device_to_add)
	{
		if (!device_to_add)
		{
			LogWarning(Channel::Devices, "Cannot register device with equipment hub; device object was invalid");
		}
		else
		{
			bool device_exists = std::any_of(
				std::execution::par_unseq,
				m_ActiveDevices.begin(),
				m_ActiveDevices.end(),
				[&device_to_add](const auto& existing_device)
				{
					return (device_to_add->DeviceId() == existing_device->DeviceId());
				}
			);

			if (device_exists)
			{
				LogDebug(Channel::Devices, "Failed to register device with equipment hub; device id already registered");
			}
			else
			{
				m_ActiveDevices.insert(std::move(device_to_add));
				return true;
			}
		}

		return false;
	}

}
// namespace AqualinkAutomate::Equipment
