#pragma once

#include <cstdint>


namespace AqualinkAutomate::Kernel
{

	enum class PumpSpeeds : uint8_t
	{
		SingleSpeed,
		TwoSpeed,
		VariableSpeed,
		Unknown
	};

}
// namespace AqualinkAutomate::Kernel
