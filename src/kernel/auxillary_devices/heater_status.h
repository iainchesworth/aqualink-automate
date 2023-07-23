#pragma once

#include <cstdint>

#include "kernel/auxillary_devices/auxillary_status.h"

namespace AqualinkAutomate::Kernel
{

	enum class HeaterStatuses : uint8_t
	{
		Off = 0x00,
		Heating = 0x01,
		Enabled = 0x04,
		NotInstalled,
		Unknown
	};

	HeaterStatuses ConvertToHeaterStatus(AuxillaryStatuses aux_status);

}
// namespace AqualinkAutomate::Kernel
