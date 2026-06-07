#include <algorithm>
#include <format>
#include <memory>
#include <ranges>
#include <typeinfo>
#include <type_traits>

#include "kernel/equipment_hub.h"

namespace AqualinkAutomate::Kernel
{

	bool EquipmentHub::EquipmentExists(std::unique_ptr<Interfaces::IEquipment> const& equipment) const
	{
		using pointer_type = std::remove_cvref_t<decltype(equipment)>;
		using equipment_type = typename pointer_type::element_type;

		std::type_index equipment_id_to_add = typeid(equipment_type);
		return m_ActiveEquipment.contains(equipment_id_to_add);
	}

	bool EquipmentHub::AddEquipment(std::unique_ptr<Interfaces::IEquipment> equipment_to_add)
	{
		if (!equipment_to_add)
		{
			LogWarning(Channel::Devices, "Cannot register equipment with equipment hub; equipment object was invalid");
		}
		else if (EquipmentExists(equipment_to_add))
		{
			LogDebug(Channel::Devices, std::format("Failed to register equipment with equipment hub; equipment id already registered"));
		}
		else if (auto [_, was_inserted] = m_ActiveEquipment.emplace(std::move(typeid(decltype(equipment_to_add)::element_type)), std::move(equipment_to_add)); !was_inserted)
		{
			LogDebug(Channel::Devices, "Failed to add equipment to equipment hub; internal error while adding equipment object");
		}
		else
		{	
			return true;
		}

		return false;
	}

	bool EquipmentHub::DeviceExists(const Interfaces::IDeviceIdentifier& device_id) const
	{
		auto is_match = [&device_id](const auto& existing_device) noexcept
			{
				return (device_id == existing_device->DeviceId());
			};

		auto match_iter = std::ranges::find_if(m_ActiveDevices, is_match);

		return (m_ActiveDevices.end() != match_iter);
	}

	bool EquipmentHub::DeviceExists(std::unique_ptr<Interfaces::IDevice> const& device) const
	{
		auto is_match = [&device](const auto& existing_device) noexcept
			{
				return (device->DeviceId() == existing_device->DeviceId());
			};

		auto match_iter = std::ranges::find_if(m_ActiveDevices, is_match);

		return (m_ActiveDevices.end() != match_iter);
	}

	bool EquipmentHub::AddDevice(std::unique_ptr<Interfaces::IDevice> device_to_add)
	{
		bool added_device_successfully = false;

		if (!device_to_add)
		{
			LogWarning(Channel::Devices, "Cannot register device with equipment hub; device object was invalid");
		}
		else if (DeviceExists(device_to_add))
		{
			LogDebug(Channel::Devices, "Failed to register device with equipment hub; device id was already registered");
		}
		else if (auto [it, inserted] = m_ActiveDevices.insert(std::move(device_to_add)); !inserted)
		{
			LogTrace(Channel::Devices, std::format("Failed to register device with with equipment hub; insertion into unordered_set failed"));
		}
		else
		{
			LogTrace(Channel::Devices, std::format("Registered device with equipment hub (active devices count = {})", m_ActiveDevices.size()));

			added_device_successfully = true;
		}

		return added_device_successfully;
	}

	Interfaces::IDevice* EquipmentHub::FindDevice(std::function<bool(const Interfaces::IDevice&)> predicate) const
	{
		for (const auto& device : m_ActiveDevices)
		{
			if (predicate(*device))
			{
				return device.get();
			}
		}

		return nullptr;
	}

}
// namespace AqualinkAutomate::Equipment
