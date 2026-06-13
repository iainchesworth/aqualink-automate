#pragma once

#include "devices/capabilities/actuation_types.h"
#include "kernel/circulation.h"

namespace AqualinkAutomate::Devices::Capabilities
{

	// Mixin advertised by any controller that can change the circulation mode
	// (Pool / Spa / Spillover). Today only the Serial Adapter drives this on the
	// wire, so this is a single-provider capability and carries no priority.
	class CirculationController
	{
	public:
		virtual ~CirculationController() = default;

		virtual ActuationResult SetCirculationMode(Kernel::CirculationModes mode) = 0;
	};

}
// namespace AqualinkAutomate::Devices::Capabilities
