#pragma once

#include <cstdint>

#include "devices/capabilities/actuation_types.h"

namespace AqualinkAutomate::Devices::Capabilities
{

	// Mixin advertised by any controller that can change the pool/spa heater
	// setpoint. The temperature arrives already converted into the system's
	// configured wire units (the dispatcher validates the range before delegating).
	// Multiple controllers may advertise this; the CommandDispatcher selects the
	// highest ControllerPriority() instance present.
	class SetpointController
	{
	public:
		virtual ~SetpointController() = default;

		virtual ActuationResult SetPoolSetpoint(uint8_t temperature) = 0;
		virtual ActuationResult SetSpaSetpoint(uint8_t temperature) = 0;

		// Precedence when several SetpointControllers are connected at once.
		virtual ActuationPriority ControllerPriority() const = 0;
	};

}
// namespace AqualinkAutomate::Devices::Capabilities
