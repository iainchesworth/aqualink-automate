#pragma once

#include <cstdint>
#include <string>

#include "devices/capabilities/actuation_types.h"

namespace AqualinkAutomate::Devices::Capabilities
{

	// Mixin advertised by any controller that can PROGRAM a spa-side switch button's
	// function over the bus (the controller's "Spa Remotes" / "Spa Switch" config UI).
	// switch_number / button_number are 1-based, as the controller numbers them; function is
	// the assignable function name exactly as the controller's picker lists it (e.g. "Pool
	// Light", "Spa Jets"). Implemented by OneTouch (menu-edit) and iAQ (page-edit) alike, so
	// the write path works for either controller. Multiple may advertise this; the highest
	// ControllerPriority() instance present is chosen.
	class SpaSwitchConfigurator
	{
	public:
		virtual ~SpaSwitchConfigurator() = default;

		virtual ActuationResult SetSpaSwitchAssignment(uint8_t switch_number, uint8_t button_number, const std::string& function) = 0;

		// Precedence when several SpaSwitchConfigurators are connected at once.
		virtual ActuationPriority ControllerPriority() const = 0;
	};

}
// namespace AqualinkAutomate::Devices::Capabilities
