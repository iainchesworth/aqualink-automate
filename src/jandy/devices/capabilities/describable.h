#pragma once

#include "interfaces/idescribable.h"

namespace AqualinkAutomate::Devices::Capabilities
{

	// Alias the core interface into the Capabilities namespace for consistency
	// with other device capabilities (Restartable, Screen, Emulated, etc.)
	using Describable = Interfaces::IDescribable;

}
// namespace AqualinkAutomate::Devices::Capabilities
