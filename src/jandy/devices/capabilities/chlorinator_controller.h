#pragma once

#include <cstdint>

#include "devices/capabilities/actuation_types.h"

namespace AqualinkAutomate::Devices::Capabilities
{

	// Mixin advertised by any controller that can drive a salt-water generator
	// (chlorinator) output percentage and boost mode. Today only the AqualinkTouch
	// (IAQ, 0x33) page UI carries chlorinator control on the wire, so this is a
	// single-provider capability and carries no priority.
	class ChlorinatorController
	{
	public:
		virtual ~ChlorinatorController() = default;

		virtual ActuationResult SetChlorinatorPercentage(uint8_t percentage) = 0;
		virtual ActuationResult SetChlorinatorBoost(bool enable) = 0;
	};

}
// namespace AqualinkAutomate::Devices::Capabilities
