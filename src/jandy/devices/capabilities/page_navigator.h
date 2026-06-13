#pragma once

#include <cstdint>

#include "devices/capabilities/actuation_types.h"

namespace AqualinkAutomate::Devices::Capabilities
{

	// Mixin advertised by a controller whose on-screen page UI can be driven by
	// selecting a page button by index (the AqualinkTouch / IAQ 0x33 panel). This
	// is the raw page-navigation primitive; higher-level actuation builds on it.
	class PageNavigator
	{
	public:
		virtual ~PageNavigator() = default;

		virtual ActuationResult ActuatePageButton(uint8_t button_index) = 0;
	};

}
// namespace AqualinkAutomate::Devices::Capabilities
