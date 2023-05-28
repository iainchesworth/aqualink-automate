#pragma once

#include <cstdint>
#include <string>

namespace AqualinkAutomate::Config
{
	
	enum class AuxillaryStates : uint8_t
	{
		On = 0x00, 
		Off = 0x01, 
		Enabled, 
		Delay
	};

	class Auxillary
	{
	public:
		std::string Label;
		AuxillaryStates State;
	};

}
// namespace AqualinkAutomate::Config
