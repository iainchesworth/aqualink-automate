#include <format>

#include <magic_enum/magic_enum.hpp>

#include "auxillaries/jandy_auxillary_traits_types.h"
#include "devices/command_dispatcher.h"
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
		auto device = m_DataHub->Devices.FindById(uuid);
		if (!device)
		{
			LogWarning(Channel::Devices, "CommandDispatcher: Device not found by UUID");
			return CommandResult::DeviceNotFound;
		}

		return DispatchToggle(device);
	}

	CommandDispatcher::CommandResult CommandDispatcher::ToggleByLabel(const std::string& label)
	{
		auto matches = m_DataHub->Devices.FindByLabel(label);
		if (matches.empty())
		{
			LogWarning(Channel::Devices, std::format("CommandDispatcher: No device found with label '{}'", label));
			return CommandResult::DeviceNotFound;
		}

		return DispatchToggle(matches.front());
	}

	CommandDispatcher::CommandResult CommandDispatcher::DispatchToggle(const std::shared_ptr<Kernel::AuxillaryDevice>& device)
	{
		// Find the SerialAdapter device using a predicate on the equipment hub.
		auto* rssa_device = m_EquipmentHub->FindDevice([](const Interfaces::IDevice& dev) -> bool
		{
			auto* jandy_type = dynamic_cast<const JandyDeviceType*>(&dev.DeviceId());
			return jandy_type && jandy_type->Class() == DeviceClasses::SerialAdapter;
		});

		if (!rssa_device)
		{
			LogWarning(Channel::Devices, "CommandDispatcher: No SerialAdapter device found in equipment hub");
			return CommandResult::NoSerialAdapter;
		}

		auto* serial_adapter = dynamic_cast<SerialAdapterDevice*>(rssa_device);
		if (!serial_adapter)
		{
			LogWarning(Channel::Devices, "CommandDispatcher: Device class is SerialAdapter but cast failed");
			return CommandResult::NoSerialAdapter;
		}

		// Determine desired action based on current device status.
		// Default to SetOn; if device is already on, use SetOff.
		auto action = SerialAdapter_CommandTypes::SetOn;

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
