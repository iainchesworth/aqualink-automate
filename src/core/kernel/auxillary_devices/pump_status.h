#pragma once

#include <cstdint>

#include "kernel/auxillary_devices/auxillary_status.h"

namespace AqualinkAutomate::Kernel
{

	enum class PumpStatuses : uint8_t
	{
		Off = 0x00,
		Running = 0x01,
		NotInstalled = 0x02,
		Unknown = 0x03
	};

	PumpStatuses ConvertToPumpStatus(AuxillaryStatuses aux_states);

}
// namespace AqualinkAutomate::Kernel
