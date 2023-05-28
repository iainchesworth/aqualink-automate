#pragma once

#include <cstdint>

namespace AqualinkAutomate::Config
{

	enum class HeaterStatus : uint8_t
	{
		Off = 0x00,
		Heating = 0x01,
		Enabled = 0x04
	};

}
// namespace AqualinkAutomate::Config
