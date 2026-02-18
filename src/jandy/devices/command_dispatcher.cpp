#include <format>

#include <magic_enum/magic_enum.hpp>

#include "auxillaries/jandy_auxillary_traits_types.h"
#include "devices/command_dispatcher.h"
#include "devices/iaq_device.h"
#include "devices/jandy_device_types.h"
#include "devices/serial_adapter_device.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes;
using namespace AqualinkAutomate::Messages;

namespace AqualinkAutomate::Devices
{

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

		return DispatchCommand(device, action);
	}

	CommandDispatcher::CommandResult CommandDispatcher::CommandByLabel(const std::string& label, DeviceAction action)
	{
		auto matches = m_DataHub->Devices.FindByLabel(label);
		if (matches.empty())
		{
			LogWarning(Channel::Devices, std::format("CommandDispatcher: No device found with label '{}'", label));
			return CommandResult::DeviceNotFound;
		}

		return DispatchCommand(matches.front(), action);
	}

	SerialAdapterDevice* CommandDispatcher::FindSerialAdapter()
	{
		auto* rssa_device = m_EquipmentHub->FindDevice([](const Interfaces::IDevice& dev) -> bool
		{
			auto* jandy_type = dynamic_cast<const JandyDeviceType*>(&dev.DeviceId());
			return jandy_type && jandy_type->Class() == DeviceClasses::SerialAdapter;
		});

		if (!rssa_device)
		{
			return nullptr;
		}

		return dynamic_cast<SerialAdapterDevice*>(rssa_device);
	}

	CommandDispatcher::CommandResult CommandDispatcher::SetPoolSetpoint(uint8_t temperature)
	{
		auto* serial_adapter = FindSerialAdapter();
		if (!serial_adapter)
		{
			LogWarning(Channel::Devices, "CommandDispatcher: No SerialAdapter device found for setpoint command");
			return CommandResult::NoSerialAdapter;
		}

		LogInfo(Channel::Devices, std::format("CommandDispatcher: Setting pool setpoint to {}", temperature));
		serial_adapter->QueueSetpointCommand(SerialAdapter_SystemTemperatureCommands::POOLSP, temperature);
		return CommandResult::Success;
	}

	CommandDispatcher::CommandResult CommandDispatcher::SetSpaSetpoint(uint8_t temperature)
	{
		auto* serial_adapter = FindSerialAdapter();
		if (!serial_adapter)
		{
			LogWarning(Channel::Devices, "CommandDispatcher: No SerialAdapter device found for setpoint command");
			return CommandResult::NoSerialAdapter;
		}

		LogInfo(Channel::Devices, std::format("CommandDispatcher: Setting spa setpoint to {}", temperature));
		serial_adapter->QueueSetpointCommand(SerialAdapter_SystemTemperatureCommands::SPASP, temperature);
		return CommandResult::Success;
	}

	IAQDevice* CommandDispatcher::FindIAQDevice()
	{
		auto* iaq_device = m_EquipmentHub->FindDevice([](const Interfaces::IDevice& dev) -> bool
		{
			auto* jandy_type = dynamic_cast<const JandyDeviceType*>(&dev.DeviceId());
			return jandy_type && jandy_type->Class() == DeviceClasses::AqualinkTouch;
		});

		if (!iaq_device)
		{
			return nullptr;
		}

		return dynamic_cast<IAQDevice*>(iaq_device);
	}

	CommandDispatcher::CommandResult CommandDispatcher::SetChlorinatorPercentage(uint8_t percentage)
	{
		if (percentage > 100)
		{
			LogWarning(Channel::Devices, std::format("CommandDispatcher: Invalid chlorinator percentage: {}", percentage));
			return CommandResult::InvalidValue;
		}

		auto* iaq_device = FindIAQDevice();
		if (!iaq_device)
		{
			LogWarning(Channel::Devices, "CommandDispatcher: No IAQ device found for chlorinator percentage command");
			return CommandResult::DeviceNotFound;
		}

		LogInfo(Channel::Devices, std::format("CommandDispatcher: Setting chlorinator percentage to {}%", percentage));
		iaq_device->QueueChlorinatorPercentage(percentage);
		return CommandResult::Success;
	}

	CommandDispatcher::CommandResult CommandDispatcher::SetChlorinatorBoost(bool enable)
	{
		auto* iaq_device = FindIAQDevice();
		if (!iaq_device)
		{
			LogWarning(Channel::Devices, "CommandDispatcher: No IAQ device found for chlorinator boost command");
			return CommandResult::DeviceNotFound;
		}

		LogInfo(Channel::Devices, std::format("CommandDispatcher: {} chlorinator boost", enable ? "Enabling" : "Disabling"));
		iaq_device->QueueChlorinatorBoost(enable);
		return CommandResult::Success;
	}

	CommandDispatcher::CommandResult CommandDispatcher::DispatchCommand(const std::shared_ptr<Kernel::AuxillaryDevice>& device, DeviceAction requested_action)
	{
		auto* serial_adapter = FindSerialAdapter();
		if (!serial_adapter)
		{
			LogWarning(Channel::Devices, "CommandDispatcher: No SerialAdapter device found in equipment hub");
			return CommandResult::NoSerialAdapter;
		}

		// Determine the serial command based on the requested action.
		auto action = SerialAdapter_CommandTypes::SetOn;

		if (requested_action == DeviceAction::Off)
		{
			action = SerialAdapter_CommandTypes::SetOff;
		}
		else if (requested_action == DeviceAction::Toggle)
		{
			// Auto-detect: default to SetOn; if device is already on, use SetOff.
			if (device->AuxillaryTraits.Has(AuxillaryTypeTrait{}))
			{
				auto device_type = *(device->AuxillaryTraits[AuxillaryTypeTrait{}]);

				switch (device_type)
				{
				case AuxillaryTypes::Auxillary:
				case AuxillaryTypes::Cleaner:
				case AuxillaryTypes::Spillover:
				case AuxillaryTypes::Sprinkler:
					if (auto status = device->AuxillaryTraits.TryGet(AuxillaryStatusTrait{}); status.has_value() && *status == Kernel::AuxillaryStatuses::On)
					{
						action = SerialAdapter_CommandTypes::SetOff;
					}
					break;

				case AuxillaryTypes::Pump:
					if (auto status = device->AuxillaryTraits.TryGet(PumpStatusTrait{}); status.has_value() && *status == Kernel::PumpStatuses::Running)
					{
						action = SerialAdapter_CommandTypes::SetOff;
					}
					break;

				default:
					break;
				}
			}
		}

		// Check if the device has a JandyAuxillaryId trait (i.e. a hardware aux port).
		if (device->AuxillaryTraits.Has(Auxillaries::JandyAuxillaryId{}))
		{
			auto aux_id = *(device->AuxillaryTraits[Auxillaries::JandyAuxillaryId{}]);
			LogInfo(Channel::Devices, std::format("CommandDispatcher: Dispatching aux command for {} (action=0x{:02x})", magic_enum::enum_name(aux_id), magic_enum::enum_integer(action)));
			serial_adapter->QueueAuxCommand(aux_id, action);
			return CommandResult::Success;
		}

		// No aux ID - try to map as a system pump command via type and label.
		if (device->AuxillaryTraits.Has(AuxillaryTypeTrait{}))
		{
			auto device_type = *(device->AuxillaryTraits[AuxillaryTypeTrait{}]);
			auto label_opt = device->AuxillaryTraits.TryGet(LabelTrait{});
			std::string label = label_opt.has_value() ? *label_opt : "";

			switch (device_type)
			{
			case AuxillaryTypes::Pump:
				if (label.find("Filter") != std::string::npos || label.find("Pump") != std::string::npos)
				{
					LogInfo(Channel::Devices, std::format("CommandDispatcher: Dispatching pump command PUMP (action=0x{:02x})", magic_enum::enum_integer(action)));
					serial_adapter->QueuePumpCommand(SerialAdapter_SystemPumpCommands::PUMP, action);
					return CommandResult::Success;
				}
				break;

			case AuxillaryTypes::Cleaner:
				LogInfo(Channel::Devices, std::format("CommandDispatcher: Dispatching pump command CLEANR (action=0x{:02x})", magic_enum::enum_integer(action)));
				serial_adapter->QueuePumpCommand(SerialAdapter_SystemPumpCommands::CLEANR, action);
				return CommandResult::Success;

			case AuxillaryTypes::Spillover:
				LogInfo(Channel::Devices, std::format("CommandDispatcher: Dispatching pump command SPILLOVER (action=0x{:02x})", magic_enum::enum_integer(action)));
				serial_adapter->QueuePumpCommand(SerialAdapter_SystemPumpCommands::SPILLOVER, action);
				return CommandResult::Success;

			default:
				break;
			}

			// Check if it's a spa device by label.
			if (label.find("Spa") != std::string::npos)
			{
				LogInfo(Channel::Devices, std::format("CommandDispatcher: Dispatching pump command SPA (action=0x{:02x})", magic_enum::enum_integer(action)));
				serial_adapter->QueuePumpCommand(SerialAdapter_SystemPumpCommands::SPA, action);
				return CommandResult::Success;
			}
		}

		LogWarning(Channel::Devices, "CommandDispatcher: Could not map device to a serial adapter command");
		return CommandResult::UnknownEquipmentType;
	}

}
// namespace AqualinkAutomate::Devices
