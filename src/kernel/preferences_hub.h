#pragma once

#include "kernel/temperature.h"

namespace AqualinkAutomate::Kernel
{

	class PreferencesHub
	{
	public:
		TemperatureUnits Temperature_DisplayUnits{ TemperatureUnits::Celsius };
	};

}
// namespace AqualinkAutomate::Kernel
