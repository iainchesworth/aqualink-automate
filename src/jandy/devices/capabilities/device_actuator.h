#pragma once

#include <memory>

#include "devices/capabilities/actuation_types.h"

namespace AqualinkAutomate::Kernel
{
	class AuxillaryDevice;
}

namespace AqualinkAutomate::Devices::Capabilities
{

	// Mixin advertised by any controller that can actuate (turn on/off/toggle) a
	// logical pool device. The controller is handed the already-resolved logical
	// device and maps it onto its own wire mechanism (Serial Adapter command bytes,
	// OneTouch menu navigation, ...). Multiple controllers may advertise this; the
	// CommandDispatcher selects the highest ControllerPriority() instance present.
	class DeviceActuator
	{
	public:
		virtual ~DeviceActuator() = default;

		virtual ActuationResult ActuateDevice(const std::shared_ptr<Kernel::AuxillaryDevice>& device, ActuationAction action) = 0;

		// Precedence when several DeviceActuators are connected at once.
		virtual ActuationPriority ControllerPriority() const = 0;
	};

}
// namespace AqualinkAutomate::Devices::Capabilities
