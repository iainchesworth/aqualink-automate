#pragma once

#include <cstdint>

#include "kernel/auxillary_devices/auxillary_status.h"

namespace AqualinkAutomate::Kernel
{

	enum class PumpStatuses : uint8_t
	{
		Off = 0x00,
		Running = 0x01,
		NotInstalled,
		Unknown
	};

	PumpStatuses ConvertToPumpStatus(AuxillaryStatuses aux_states);

}
// namespace AqualinkAutomate::Kernel
