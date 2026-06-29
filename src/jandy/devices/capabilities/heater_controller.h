#pragma once

#include "devices/capabilities/actuation_types.h"
#include "kernel/body_of_water_ids.h"

namespace AqualinkAutomate::Devices::Capabilities
{

	// Mixin advertised by any controller that can enable/disable a heater (turn its
	// thermostat on or off; the controller itself decides when to actually fire based on
	// the body temperature vs setpoint). The heater is identified by its body of water:
	// Pool -> pool heater, Spa -> spa heater, Shared -> the solar heater. Today only the
	// Serial Adapter drives this on the wire, so it carries no priority. The wire encoding is
	// validated live (see SerialAdapterDevice::QueueHeaterCommand).
	class HeaterController
	{
	public:
		virtual ~HeaterController() = default;

		virtual ActuationResult SetHeaterMode(Kernel::BodyOfWaterIds heater_body, bool enable) = 0;
	};

}
// namespace AqualinkAutomate::Devices::Capabilities
