#pragma once

#include <cstdint>

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

}
// namespace AqualinkAutomate::Kernel
