#include <format>

#include "devices/capabilities/chlorinator_controller.h"
#include "devices/capabilities/circulation_controller.h"
#include "devices/capabilities/device_actuator.h"
#include "devices/capabilities/page_navigator.h"
#include "devices/capabilities/setpoint_controller.h"
#include "devices/command_dispatcher.h"
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

		// Map a controller's ActuationResult back onto the dispatcher's CommandResult.
		// `when_unsupported` is what to return when the controller reports the action
		// is not supported -- chosen per call-site to PRESERVE the exact historical
		// result code for that command kind (e.g. toggles/setpoints reported
		// NoSerialAdapter; chlorinator/page reported DeviceNotFound).
		Interfaces::ICommandDispatcher::CommandResult MapActuationResult(Capabilities::ActuationResult result, Interfaces::ICommandDispatcher::CommandResult when_unsupported)
		{
			using CR = Interfaces::ICommandDispatcher::CommandResult;

			switch (result)
			{
			case Capabilities::ActuationResult::Accepted:      return CR::Success;
			case Capabilities::ActuationResult::InvalidValue:  return CR::InvalidValue;
			case Capabilities::ActuationResult::MappingFailed: return CR::UnknownEquipmentType;
			case Capabilities::ActuationResult::NotSupported:  return when_unsupported;
			}

			return when_unsupported;
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

		auto* actuator = FindCapable<Capabilities::DeviceActuator>();
		if (nullptr == actuator)
		{
			LogWarning(Channel::Devices, "CommandDispatcher: No connected controller can actuate equipment");
			return CommandResult::NoSerialAdapter;
		}

		return MapActuationResult(actuator->ActuateDevice(device, ToActuationAction(action)), CommandResult::NoSerialAdapter);
	}

	CommandDispatcher::CommandResult CommandDispatcher::CommandByLabel(const std::string& label, DeviceAction action)
	{
		auto matches = m_DataHub->Devices.FindByLabel(label);
		if (matches.empty())
		{
			LogWarning(Channel::Devices, std::format("CommandDispatcher: No device found with label '{}'", label));
			return CommandResult::DeviceNotFound;
		}

		auto* actuator = FindCapable<Capabilities::DeviceActuator>();
		if (nullptr == actuator)
		{
			LogWarning(Channel::Devices, "CommandDispatcher: No connected controller can actuate equipment");
			return CommandResult::NoSerialAdapter;
		}

		return MapActuationResult(actuator->ActuateDevice(matches.front(), ToActuationAction(action)), CommandResult::NoSerialAdapter);
	}

	CommandDispatcher::CommandResult CommandDispatcher::SetPoolSetpoint(uint8_t temperature)
	{
		if ((temperature < SETPOINT_MIN_VALUE) || (temperature > SETPOINT_MAX_VALUE))
		{
			LogWarning(Channel::Devices, std::format("CommandDispatcher: Pool setpoint {} out of valid range [{}, {}]", static_cast<int>(temperature), static_cast<int>(SETPOINT_MIN_VALUE), static_cast<int>(SETPOINT_MAX_VALUE)));
			return CommandResult::InvalidValue;
		}

		auto* controller = FindCapable<Capabilities::SetpointController>();
		if (nullptr == controller)
		{
			LogWarning(Channel::Devices, "CommandDispatcher: No connected controller can set a setpoint");
			return CommandResult::NoSerialAdapter;
		}

		LogInfo(Channel::Devices, std::format("CommandDispatcher: Setting pool setpoint to {}", temperature));
		return MapActuationResult(controller->SetPoolSetpoint(temperature), CommandResult::NoSerialAdapter);
	}

	CommandDispatcher::CommandResult CommandDispatcher::SetSpaSetpoint(uint8_t temperature)
	{
		if ((temperature < SETPOINT_MIN_VALUE) || (temperature > SETPOINT_MAX_VALUE))
		{
			LogWarning(Channel::Devices, std::format("CommandDispatcher: Spa setpoint {} out of valid range [{}, {}]", static_cast<int>(temperature), static_cast<int>(SETPOINT_MIN_VALUE), static_cast<int>(SETPOINT_MAX_VALUE)));
			return CommandResult::InvalidValue;
		}

		auto* controller = FindCapable<Capabilities::SetpointController>();
		if (nullptr == controller)
		{
			LogWarning(Channel::Devices, "CommandDispatcher: No connected controller can set a setpoint");
			return CommandResult::NoSerialAdapter;
		}

		LogInfo(Channel::Devices, std::format("CommandDispatcher: Setting spa setpoint to {}", temperature));
		return MapActuationResult(controller->SetSpaSetpoint(temperature), CommandResult::NoSerialAdapter);
	}

	CommandDispatcher::CommandResult CommandDispatcher::SetChlorinatorPercentage(uint8_t percentage)
	{
		if (percentage > 100)
		{
			LogWarning(Channel::Devices, std::format("CommandDispatcher: Invalid chlorinator percentage: {}", percentage));
			return CommandResult::InvalidValue;
		}

		auto* controller = FindCapable<Capabilities::ChlorinatorController>();
		if (nullptr == controller)
		{
			LogWarning(Channel::Devices, "CommandDispatcher: No connected controller can drive the chlorinator");
			return CommandResult::DeviceNotFound;
		}

		LogInfo(Channel::Devices, std::format("CommandDispatcher: Setting chlorinator percentage to {}%", percentage));
		return MapActuationResult(controller->SetChlorinatorPercentage(percentage), CommandResult::DeviceNotFound);
	}

	CommandDispatcher::CommandResult CommandDispatcher::SetChlorinatorBoost(bool enable)
	{
		auto* controller = FindCapable<Capabilities::ChlorinatorController>();
		if (nullptr == controller)
		{
			LogWarning(Channel::Devices, "CommandDispatcher: No connected controller can drive the chlorinator");
			return CommandResult::DeviceNotFound;
		}

		LogInfo(Channel::Devices, std::format("CommandDispatcher: {} chlorinator boost", enable ? "Enabling" : "Disabling"));
		return MapActuationResult(controller->SetChlorinatorBoost(enable), CommandResult::DeviceNotFound);
	}

	CommandDispatcher::CommandResult CommandDispatcher::SelectIAQPageButton(uint8_t button_index)
	{
		auto* navigator = FindCapable<Capabilities::PageNavigator>();
		if (nullptr == navigator)
		{
			LogWarning(Channel::Devices, "CommandDispatcher: No connected controller exposes a page UI to navigate");
			return CommandResult::DeviceNotFound;
		}

		LogInfo(Channel::Devices, std::format("CommandDispatcher: Selecting IAQ page button index {}", static_cast<int>(button_index)));
		return MapActuationResult(navigator->ActuatePageButton(button_index), CommandResult::DeviceNotFound);
	}

	CommandDispatcher::CommandResult CommandDispatcher::SetCirculationMode(Kernel::CirculationModes mode)
	{
		auto* controller = FindCapable<Capabilities::CirculationController>();
		if (nullptr == controller)
		{
			LogWarning(Channel::Devices, "CommandDispatcher: No connected controller can set the circulation mode");
			return CommandResult::NoSerialAdapter;
		}

		return MapActuationResult(controller->SetCirculationMode(mode), CommandResult::NoSerialAdapter);
	}

}
// namespace AqualinkAutomate::Devices
