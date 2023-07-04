#pragma once

#include <cstdint>

namespace AqualinkAutomate::Kernel
{

	enum class AuxillaryStates : uint8_t
	{
		On = 0x00,
		Off = 0x01,
		Enabled,
		Pending,
		Unknown
	};

}
// namespace AqualinkAutomate::Kernel