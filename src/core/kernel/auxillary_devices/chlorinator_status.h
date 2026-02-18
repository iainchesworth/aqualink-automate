#pragma once

#include <cstdint>

#include "kernel/auxillary_devices/auxillary_status.h"

namespace AqualinkAutomate::Kernel
{

	enum class ChlorinatorStatuses : uint8_t
	{
		Off,
		On,
		TurningOff,
		Warning_NoFlow,
		Warning_LowSalt,
		Warning_HighSalt,
		Warning_HighCurrent,
		Warning_CleanCell,
		Warning_LowVoltage,
		Warning_LowTemperature,
		Error_CheckPCB,
		GeneralFault,
		Unknown
	};

	ChlorinatorStatuses ConvertToChlorinatorStatus(AuxillaryStatuses aux_states);

}
// namespace AqualinkAutomate::Kernel
