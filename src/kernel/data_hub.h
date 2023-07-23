#pragma once

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

#include <boost/functional/hash.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/signals2.hpp>

#include "jandy/equipment/jandy_equipment_modes.h"
#include "jandy/equipment/jandy_equipment_versions.h"
#include "jandy/utility/string_conversion/timeout_duration.h"
#include "kernel/circulation.h"
#include "kernel/data_hub_event.h"
#include "kernel/device_graph/device_graph.h"
#include "kernel/orp.h"
#include "kernel/ph.h"
#include "kernel/pool_configurations.h"
#include "kernel/powercenter.h"
#include "kernel/temperature.h"
#include "kernel/system_boards.h"
#include "kernel/auxillary_devices/auxillary_device.h"
#include "logging/logging.h"
#include "types/units_dimensionless.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Kernel
{

	class DataHub
	{
	public:
		DataHub();

	//---------------------------------------------------------------------
	// UPDATES / NOTIFICATIONS / EVENTS
	//---------------------------------------------------------------------

	public:
		mutable boost::signals2::signal<void(std::shared_ptr<Kernel::DataHub_Event>)> ConfigUpdateSignal;

	//---------------------------------------------------------------------
	// EQUIPMENT CONFIGURATION
	//---------------------------------------------------------------------

	public:
		Kernel::PoolConfigurations PoolConfiguration{ Kernel::PoolConfigurations::Unknown };
		Kernel::SystemBoards SystemBoard{ Kernel::SystemBoards::Unknown };

	//---------------------------------------------------------------------
	// EQUIPMENT STATUS
	//---------------------------------------------------------------------

	public:
		Equipment::JandyEquipmentModes Mode{ Equipment::JandyEquipmentModes::Normal };
		Utility::TimeoutDuration TimeoutRemaining;
		std::chrono::year_month_day Date{ std::chrono::year{2000}, std::chrono::month{1}, std::chrono::day{1} };
		std::chrono::hh_mm_ss<std::chrono::milliseconds> Time{ std::chrono::milliseconds(0) };

	//---------------------------------------------------------------------
	// EQUIPMENT VERSIONS
	//---------------------------------------------------------------------

	public:
		Equipment::JandyEquipmentVersions EquipmentVersions;

	//---------------------------------------------------------------------
	// CIRCULATION MODES
	//---------------------------------------------------------------------

	public:
		CirculationModes CirculationMode{ CirculationModes::Pool };

		bool SpaMode() const
		{
			return (CirculationModes::Spa == CirculationMode);
		}

		bool InCleanMode{ false };

	//---------------------------------------------------------------------
	// TEMPERATURES
	//---------------------------------------------------------------------

	public:
		Kernel::Temperature AirTemp() const;
		Kernel::Temperature PoolTemp() const;
		Kernel::Temperature SpaTemp() const;
		Kernel::Temperature FreezeProtectPoint() const;

	public:
		void AirTemp(const Kernel::Temperature& air_temp);
		void PoolTemp(const Kernel::Temperature& pool_temp);
		void SpaTemp(const Kernel::Temperature& spa_temp);
		void FreezeProtectPoint(const Kernel::Temperature& freeze_protect_point);

	private:
		Kernel::Temperature m_AirTemp{ Kernel::Temperature::ConvertToTemperatureInCelsius(0.0f) };
		Kernel::Temperature m_PoolTemp{ Kernel::Temperature::ConvertToTemperatureInCelsius(0.0f) };
		Kernel::Temperature m_SpaTemp{ Kernel::Temperature::ConvertToTemperatureInCelsius(0.0f) };
		Kernel::Temperature m_FreezeProtectPoint{ Kernel::Temperature::ConvertToTemperatureInCelsius(0.0f) };

	//---------------------------------------------------------------------
	// CHEMISTRY
	//---------------------------------------------------------------------
		 
	public:
		Kernel::ORP ORP() const;
		Kernel::pH pH() const;
		ppm_quantity SaltLevel() const;

	public:
		void ORP(const Kernel::ORP& orp);
		void pH(const Kernel::pH& pH);
		void SaltLevel(const ppm_quantity& salt_level_in_ppm);

	private:
		Kernel::ORP m_ORP{ 0.0f };
		Kernel::pH m_pH{ 0.0f };
		ppm_quantity m_SaltLevel{ 0 };

	//---------------------------------------------------------------------
	// DEVICES GRAPH
	//---------------------------------------------------------------------

	public:
		DevicesGraph Devices{};

	public:
		std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> Auxillaries() const;
		std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> Chlorinators() const;
		std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> Heaters() const;
		std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> Pumps() const;

	public:
		std::optional<std::shared_ptr<Kernel::AuxillaryDevice>> FilterPump();
	};

}
// namespace AqualinkAutomate::Equipment
