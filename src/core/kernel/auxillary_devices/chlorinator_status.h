#pragma once

#include <cstdint>

#include "kernel/auxillary_devices/auxillary_status.h"

namespace AqualinkAutomate::Kernel
{

	enum class ChlorinatorStatuses : uint8_t
	{
		Off,
		Running,
		Unknown
	};

	ChlorinatorStatuses ConvertToChlorinatorStatus(AuxillaryStatuses aux_states);

}
// namespace AqualinkAutomate::Kernel
