#pragma once

#include <cstdint>

namespace AqualinkAutomate::Serial
{

	enum class Parity : uint8_t
	{
		None = 0,
		Odd = 1,
		Even = 2
	};

	enum class StopBits : uint8_t
	{
		One = 0,
		OnePointFive = 1,
		Two = 2
	};

	enum class FlowControl : uint8_t
	{
		None = 0,
		Software = 1,
		Hardware = 2
	};

}
// namespace AqualinkAutomate::Serial
