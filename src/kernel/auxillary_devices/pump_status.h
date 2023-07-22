#pragma once

#include <cstdint>

namespace AqualinkAutomate::Kernel
{

	enum class PumpStatuses : uint8_t
	{
		Off = 0x00,
		Running = 0x01,
		NotInstalled,
		Unknown
	};

}
// namespace AqualinkAutomate::Kernel
