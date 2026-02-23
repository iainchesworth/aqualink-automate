#pragma once

#include "interfaces/ihub.h"
#include "kernel/temperature.h"

namespace AqualinkAutomate::Kernel
{

	class PreferencesHub : public Interfaces::IHub
	{
	public:
		PreferencesHub();
		~PreferencesHub() override = default;

	public:
		TemperatureUnits Temperature_DisplayUnits{ TemperatureUnits::Celsius };
	};

}
// namespace AqualinkAutomate::Kernel
