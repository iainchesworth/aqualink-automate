#include <format>
#include <memory>
#include <string>
#include <string_view>

#include <magic_enum/magic_enum.hpp>

#include "devices/capabilities/chlorinator_controller.h"
#include "devices/capabilities/circulation_controller.h"
#include "devices/capabilities/device_actuator.h"
#include "devices/capabilities/page_navigator.h"
#include "devices/capabilities/setpoint_controller.h"
#include "devices/command_dispatcher.h"
#include "kernel/auxillary_traits/auxillary_traits_helpers.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	namespace
	{
		Capabilities::ActuationAction ToActuationAction(Interfaces::ICommandDispatcher::DeviceAction action)
		{
			switch (action)
			{
			case Interfaces::ICommandDispatcher::DeviceAction::On:  return Capabilities::ActuationAction::On;
			case Interfaces::ICommandDispatcher::DeviceAction::Off: return Capabilities::ActuationAction::Off;
			default:                                                return Capabilities::ActuationAction::Toggle;
			}
		}

		std::string_view ActionVerb(Interfaces::ICommandDispatcher::DeviceAction action)
		{
			switch (action)
			{
			case Interfaces::ICommandDispatcher::DeviceAction::On:  return "turn on";
			case Interfaces::ICommandDispatcher::DeviceAction::Off: return "turn off";
			default:                                                return "toggle";
			}
		}

		std::string DeviceLabel(const std::shared_ptr<Kernel::AuxillaryDevice>& device)
		{
			if (device && device->AuxillaryTraits.Has(Kernel::AuxillaryTraitsTypes::LabelTrait{}))
			{
				return *(device->AuxillaryTraits[Kernel::AuxillaryTraitsTypes::LabelTrait{}]);
			}

			return "equipment";
		}
	}
	// unnamed namespace

	CommandDispatcher::CommandDispatcher(std::shared_ptr<Kernel::DataHub> data_hub, std::shared_ptr<Kernel::EquipmentHub> equipment_hub)
		: m_DataHub(std::move(data_hub))
		, m_EquipmentHub(std::move(equipment_hub))
	{
	}

	CommandDispatcher::CommandResult CommandDispatcher::ToggleByUuid(const boost::uuids::uuid& uuid)
	{
		return CommandByUuid(uuid, DeviceAction::Toggle);
	}

	CommandDispatcher::CommandResult CommandDispatcher::ToggleByLabel(const std::string& label)
	{
		return CommandByLabel(label, DeviceAction::Toggle);
	}

	CommandDispatcher::CommandResult CommandDispatcher::CommandByUuid(const boost::uuids::uuid& uuid, DeviceAction action)
	{
		auto device = m_DataHub->Devices.FindById(uuid);
		if (!device)
		{
			LogWarning(Channel::Devices, "CommandDispatcher: Device not found by UUID");
			return CommandResult::DeviceNotFound;
		}

		const std::string what{ std::format("{} '{}'", ActionVerb(action), DeviceLabel(device)) };
		return DispatchToCapable<Capabilities::DeviceActuator>(
			what,
			[&](Capabilities::DeviceActuator& actuator) { return actuator.ActuateDevice(device, ToActuationAction(action)); },
			CommandResult::NoSerialAdapter);
	}

	CommandDispatcher::CommandResult CommandDispatcher::CommandByLabel(const std::string& label, DeviceAction action)
	{
		auto matches = m_DataHub->Devices.FindByLabel(label);
		if (matches.empty())
		{
			LogWarning(Channel::Devices, std::format("CommandDispatcher: No device found with label '{}'", label));
			return CommandResult::DeviceNotFound;
		}

		auto device = matches.front();
		const std::string what{ std::format("{} '{}'", ActionVerb(action), label) };
		return DispatchToCapable<Capabilities::DeviceActuator>(
			what,
			[&](Capabilities::DeviceActuator& actuator) { return actuator.ActuateDevice(device, ToActuationAction(action)); },
			CommandResult::NoSerialAdapter);
	}

	CommandDispatcher::CommandResult CommandDispatcher::SetPoolSetpoint(uint8_t temperature)
	{
		if ((temperature < SETPOINT_MIN_VALUE) || (temperature > SETPOINT_MAX_VALUE))
		{
			LogWarning(Channel::Devices, std::format("CommandDispatcher: Pool setpoint {} out of valid range [{}, {}]", static_cast<int>(temperature), static_cast<int>(SETPOINT_MIN_VALUE), static_cast<int>(SETPOINT_MAX_VALUE)));
			return CommandResult::InvalidValue;
		}

		LogInfo(Channel::Devices, std::format("CommandDispatcher: Setting pool setpoint to {}", temperature));
		return DispatchToCapable<Capabilities::SetpointController>(
			std::format("set the pool setpoint to {}", temperature),
			[&](Capabilities::SetpointController& controller) { return controller.SetPoolSetpoint(temperature); },
			CommandResult::NoSerialAdapter);
	}

	CommandDispatcher::CommandResult CommandDispatcher::SetSpaSetpoint(uint8_t temperature)
	{
		if ((temperature < SETPOINT_MIN_VALUE) || (temperature > SETPOINT_MAX_VALUE))
		{
			LogWarning(Channel::Devices, std::format("CommandDispatcher: Spa setpoint {} out of valid range [{}, {}]", static_cast<int>(temperature), static_cast<int>(SETPOINT_MIN_VALUE), static_cast<int>(SETPOINT_MAX_VALUE)));
			return CommandResult::InvalidValue;
		}

		LogInfo(Channel::Devices, std::format("CommandDispatcher: Setting spa setpoint to {}", temperature));
		return DispatchToCapable<Capabilities::SetpointController>(
			std::format("set the spa setpoint to {}", temperature),
			[&](Capabilities::SetpointController& controller) { return controller.SetSpaSetpoint(temperature); },
			CommandResult::NoSerialAdapter);
	}

	CommandDispatcher::CommandResult CommandDispatcher::SetChlorinatorPercentage(uint8_t percentage)
	{
		if (percentage > 100)
		{
			LogWarning(Channel::Devices, std::format("CommandDispatcher: Invalid chlorinator percentage: {}", percentage));
			return CommandResult::InvalidValue;
		}

		LogInfo(Channel::Devices, std::format("CommandDispatcher: Setting chlorinator percentage to {}%", percentage));
		return DispatchToCapable<Capabilities::ChlorinatorController>(
			std::format("set the chlorinator to {}%", percentage),
			[&](Capabilities::ChlorinatorController& controller) { return controller.SetChlorinatorPercentage(percentage); },
			CommandResult::DeviceNotFound);
	}

	CommandDispatcher::CommandResult CommandDispatcher::SetChlorinatorBoost(bool enable)
	{
		LogInfo(Channel::Devices, std::format("CommandDispatcher: {} chlorinator boost", enable ? "Enabling" : "Disabling"));
		return DispatchToCapable<Capabilities::ChlorinatorController>(
			std::format("{} chlorinator boost", enable ? "enable" : "disable"),
			[&](Capabilities::ChlorinatorController& controller) { return controller.SetChlorinatorBoost(enable); },
			CommandResult::DeviceNotFound);
	}

	CommandDispatcher::CommandResult CommandDispatcher::SelectIAQPageButton(uint8_t button_index)
	{
		LogInfo(Channel::Devices, std::format("CommandDispatcher: Selecting IAQ page button index {}", static_cast<int>(button_index)));
		return DispatchToCapable<Capabilities::PageNavigator>(
			std::format("select page button {}", static_cast<int>(button_index)),
			[&](Capabilities::PageNavigator& navigator) { return navigator.ActuatePageButton(button_index); },
			CommandResult::DeviceNotFound);
	}

	CommandDispatcher::CommandResult CommandDispatcher::SetCirculationMode(Kernel::CirculationModes mode)
	{
		return DispatchToCapable<Capabilities::CirculationController>(
			std::format("set circulation mode to {}", magic_enum::enum_name(mode)),
			[&](Capabilities::CirculationController& controller) { return controller.SetCirculationMode(mode); },
			CommandResult::NoSerialAdapter);
	}

}
// namespace AqualinkAutomate::Devices
