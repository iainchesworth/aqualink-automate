#pragma once

#include "kernel/hub_locator.h"
#include "options/options_settings.h"

namespace AqualinkAutomate::Pentair
{

	void Initialise(Kernel::HubLocator& hub_locator);
	void Configure(Kernel::HubLocator& hub_locator, const AqualinkAutomate::Options::Settings& settings);

}
// namespace AqualinkAutomate::Pentair
