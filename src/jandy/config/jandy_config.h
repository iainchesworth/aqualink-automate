#pragma once

#include <string>

#include "jandy/equipment/jandy_equipment_modes.h"
#include "jandy/equipment/jandy_equipment_versions.h"
#include "jandy/utility/temperature.h"
#include "jandy/utility/timeout_duration.h"

namespace AqualinkAutomate::Config
{

	class JandyConfig
	{
	public:
		Equipment::JandyEquipmentModes Mode;
		Utility::TimeoutDuration TimeoutRemaining;

	public:
		Utility::Temperature AirTemp;
		Utility::Temperature PoolTemp;
		Utility::Temperature SpaTemp;
		Utility::Temperature FreezeProtectPoint;

	public:
		Equipment::JandyEquipmentVersions EquipmentVersions;
	};

}
// namespace AqualinkAutomate::Equipment
