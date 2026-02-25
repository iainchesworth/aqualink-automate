#pragma once

#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/functional/hash.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/signals2.hpp>

#include "interfaces/ihub.h"
#include "kernel/body_of_water.h"
#include "kernel/body_of_water_ids.h"
#include "kernel/circulation.h"
#include "kernel/equipment_versions.h"
#include "kernel/hub_events/data_hub_config_event.h"
#include "utility/timeout_duration_string_converter.h"
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

	enum class EquipmentMode
	{
		Normal,
		Service,
		TimeOut
	};

	enum class ConfigurationSource
	{
		Auto,
		UserSpecified
	};

	class DataHub : public Interfaces::IHub
	{
	public:
		DataHub();

	//---------------------------------------------------------------------
	// UPDATES / NOTIFICATIONS / EVENTS
	//---------------------------------------------------------------------

	public:
		mutable boost::signals2::signal<void(std::shared_ptr<Kernel::DataHub_ConfigEvent>)> ConfigUpdateSignal;

	//---------------------------------------------------------------------
	// EQUIPMENT CONFIGURATION
	//---------------------------------------------------------------------

	public:
		Kernel::EquipmentMode Mode{ Kernel::EquipmentMode::Normal };
		Kernel::PoolConfigurations PoolConfiguration{ Kernel::PoolConfigurations::Unknown };
		Kernel::ConfigurationSource PoolConfigurationSource{ Kernel::ConfigurationSource::Auto };
		Kernel::SystemBoards SystemBoard{ Kernel::SystemBoards::Unknown };
		bool EmulationDisabled{ false };

	//---------------------------------------------------------------------
	// EQUIPMENT STATUS
	//---------------------------------------------------------------------

	public:
		Utility::TimeoutDurationStringConverter TimeoutRemaining;
		std::chrono::year_month_day Date{ std::chrono::year{2000}, std::chrono::month{1}, std::chrono::day{1} };
		std::chrono::hh_mm_ss<std::chrono::milliseconds> Time{ std::chrono::milliseconds(0) };

	//---------------------------------------------------------------------
	// EQUIPMENT VERSIONS
	//---------------------------------------------------------------------

	public:
		Kernel::EquipmentVersions EquipmentVersions;

	//---------------------------------------------------------------------
	// CIRCULATION MODES
	//---------------------------------------------------------------------

	public:
		CirculationModes CirculationMode{ CirculationModes::Pool };

		bool SpaMode() const
		{
			return CirculationModes::Spa == CirculationMode
				|| CirculationModes::SpaFill == CirculationMode
				|| CirculationModes::SpaDrain == CirculationMode;
		}

		bool InCleanMode{ false };

	//---------------------------------------------------------------------
	// BODIES OF WATER
	//---------------------------------------------------------------------

	public:
		void ApplyPoolConfiguration(PoolConfigurations config, ConfigurationSource source = ConfigurationSource::UserSpecified);
		void AddBody(BodyOfWater body);
		std::optional<std::reference_wrapper<BodyOfWater>> GetBody(BodyOfWaterIds id);
		std::optional<std::reference_wrapper<const BodyOfWater>> GetBody(BodyOfWaterIds id) const;
		std::optional<std::reference_wrapper<BodyOfWater>> ActiveBody();
		const std::vector<BodyOfWater>& Bodies() const;

	private:
		std::vector<BodyOfWater> m_Bodies;

	//---------------------------------------------------------------------
	// TEMPERATURES
	//---------------------------------------------------------------------

	public:
		std::optional<Kernel::Temperature> AirTemp() const;
		std::optional<Kernel::Temperature> PoolTemp() const;
		std::optional<Kernel::Temperature> SpaTemp() const;
		std::optional<Kernel::Temperature> PoolTempSetpoint() const;
		std::optional<Kernel::Temperature> SpaTempSetpoint() const;
		std::optional<Kernel::Temperature> FreezeProtectPoint() const;
		Kernel::TemperatureUnits SystemTemperatureUnits() const;

	public:
		void AirTemp(const Kernel::Temperature& air_temp);
		void PoolTemp(const Kernel::Temperature& pool_temp);
		void SpaTemp(const Kernel::Temperature& spa_temp);
		void PoolTempSetpoint(const Kernel::Temperature& pool_temp_setpoint);
		void SpaTempSetpoint(const Kernel::Temperature& spa_temp_setpoint);
		void FreezeProtectPoint(const Kernel::Temperature& freeze_protect_point);
		void SystemTemperatureUnits(Kernel::TemperatureUnits units);

	private:
		std::optional<Kernel::Temperature> m_AirTemp;
		std::optional<Kernel::Temperature> m_PoolTemp;
		std::optional<Kernel::Temperature> m_SpaTemp;
		std::optional<Kernel::Temperature> m_PoolTempSetpoint;
		std::optional<Kernel::Temperature> m_SpaTempSetpoint;
		std::optional<Kernel::Temperature> m_FreezeProtectPoint;
		Kernel::TemperatureUnits m_SystemTemperatureUnits{ Kernel::TemperatureUnits::Fahrenheit };

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
		std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> FilterPumps() const;

	public:
		[[deprecated("Use FilterPumps() instead; that returns a collection of pumps as there might be more than one")]]
		std::optional<std::shared_ptr<Kernel::AuxillaryDevice>> FilterPump();

	};

}
// namespace AqualinkAutomate::Equipment
