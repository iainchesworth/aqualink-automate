#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

#include "jandy/config/jandy_config_auxillary.h"
#include "jandy/config/jandy_config_circulation.h"
#include "jandy/config/jandy_config_heater.h"
#include "jandy/config/jandy_config_remotepowercenter.h"
#include "jandy/equipment/jandy_equipment_modes.h"
#include "jandy/equipment/jandy_equipment_versions.h"
#include "jandy/utility/temperature.h"
#include "jandy/utility/timeout_duration.h"

namespace AqualinkAutomate::Config
{

	class JandyConfig
	{
	public:
		Equipment::JandyEquipmentModes Mode{ Equipment::JandyEquipmentModes::Normal };
		Utility::TimeoutDuration TimeoutRemaining;
		std::chrono::year_month_day Date;
		std::chrono::hh_mm_ss<std::chrono::milliseconds> Time;

	public:
		CirculationModes CirculationMode{ CirculationModes::Pool };

		bool SpaMode() const
		{
			return (CirculationModes::Spa == CirculationMode);
		}

		bool InCleanMode;

	public:
		HeaterStatus PoolHeat{ HeaterStatus::Off };
		HeaterStatus SpaHeat{ HeaterStatus::Off };
		HeaterStatus SolarHeat{ HeaterStatus::Off }; // Or HeatPump

	public:
		Auxillary FilterPump{ "Filter Pump", AuxillaryStates::Off };
		Auxillary Aux1{ "Aux1", AuxillaryStates::Off }; // Or Cleaner
		Auxillary Aux2{ "Aux2", AuxillaryStates::Off };
		Auxillary Aux3{ "Aux3", AuxillaryStates::Off }; // Or Spillover
		Auxillary Aux4{ "Aux4", AuxillaryStates::Off };
		Auxillary Aux5{ "Aux5", AuxillaryStates::Off };
		Auxillary Aux6{ "Aux6", AuxillaryStates::Off };
		Auxillary Aux7{ "Aux7", AuxillaryStates::Off };

	public:
		std::unordered_map<RemotePowerCenterIds, RemotePowerCenter> RemotePowerCenters;

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
