#pragma once

#include <cstdint>

namespace AqualinkAutomate::Kernel
{

	enum class AuxillaryStatuses : uint8_t
	{
		On = 0x00,
		Off = 0x01,
		Enabled = 0x02,
		Pending = 0x03,
		Unknown = 0x04
	};

}
// namespace AqualinkAutomate::Kernel
