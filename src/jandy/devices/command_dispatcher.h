#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <boost/uuid/uuid.hpp>

#include "devices/capabilities/actuation_types.h"
#include "interfaces/icommanddispatcher.h"
#include "interfaces/idevice.h"
#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/auxillary_devices/auxillary_device.h"

namespace AqualinkAutomate::Devices
{

	class CommandDispatcher : public Interfaces::ICommandDispatcher
	{
	public:
		// Sane absolute bounds for a pool/spa water-temperature setpoint as it reaches the wire.
		//
		// The setpoint value arrives here already converted into the system's configured units
		// (Fahrenheit or Celsius) by the caller, so this guard must accommodate both: valid
		// Celsius setpoints (~4-40C) and valid Fahrenheit setpoints (~40-104F) both fall inside
		// [SETPOINT_MIN_VALUE, SETPOINT_MAX_VALUE]. The range deliberately rejects the 0 value
		// (sensor-unavailable sentinel) and anything above the maximum Fahrenheit setpoint so an
		// attacker-controlled HTTP/MQTT payload cannot push an absurd value onto the RS-485 bus.
		static constexpr uint8_t SETPOINT_MIN_VALUE{ 1 };
		static constexpr uint8_t SETPOINT_MAX_VALUE{ 104 };

		CommandDispatcher(std::shared_ptr<Kernel::DataHub> data_hub, std::shared_ptr<Kernel::EquipmentHub> equipment_hub);

		CommandResult ToggleByUuid(const boost::uuids::uuid& uuid) override;
		CommandResult ToggleByLabel(const std::string& label) override;
		CommandResult CommandByUuid(const boost::uuids::uuid& uuid, DeviceAction action) override;
		CommandResult CommandByLabel(const std::string& label, DeviceAction action) override;
		CommandResult SetPoolSetpoint(uint8_t temperature) override;
		CommandResult SetSpaSetpoint(uint8_t temperature) override;
		CommandResult SetChlorinatorPercentage(uint8_t percentage) override;
		CommandResult SetChlorinatorBoost(bool enable) override;
		CommandResult SetCirculationMode(Kernel::CirculationModes mode) override;
		CommandResult SelectIAQPageButton(uint8_t button_index) override;

	private:
		// Find the connected controller that advertises capability Cap, routing the
		// command to whatever controller is actually running (Serial Adapter, IAQ /
		// AqualinkTouch, OneTouch, ...). When several controllers advertise the same
		// capability (e.g. both a Serial Adapter and an emulated OneTouch can actuate
		// equipment), the highest-priority one wins where the capability exposes
		// ControllerPriority(); single-provider capabilities take the first found.
		template<typename Cap>
		Cap* FindCapable() const
		{
			Cap* best{ nullptr };
			[[maybe_unused]] Capabilities::ActuationPriority best_priority{ Capabilities::ActuationPriority::Lowest };

			m_EquipmentHub->ForEachDevice([&](Interfaces::IDevice& device)
			{
				auto* candidate = dynamic_cast<Cap*>(&device);
				if (nullptr == candidate)
				{
					return;
				}

				if constexpr (requires { candidate->ControllerPriority(); })
				{
					const auto priority = candidate->ControllerPriority();
					if ((nullptr == best) || (static_cast<int>(priority) < static_cast<int>(best_priority)))
					{
						best = candidate;
						best_priority = priority;
					}
				}
				else if (nullptr == best)
				{
					best = candidate;
				}
			});

			return best;
		}

	private:
		std::shared_ptr<Kernel::DataHub> m_DataHub;
		std::shared_ptr<Kernel::EquipmentHub> m_EquipmentHub;
	};

}
// namespace AqualinkAutomate::Devices
