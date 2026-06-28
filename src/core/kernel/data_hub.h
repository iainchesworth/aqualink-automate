#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <boost/signals2.hpp>

#include "interfaces/ihub.h"
#include "kernel/body_of_water.h"
#include "kernel/body_of_water_ids.h"
#include "kernel/circulation.h"
#include "kernel/equipment_validation.h"
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

	// Forward declarations for the concrete config-update event types populated
	// by the temperature/chemistry emit helpers. The full definitions are only
	// required in data_hub.cpp where the helpers are defined.
	class DataHub_ConfigEvent_Temperature;
	class DataHub_ConfigEvent_Chemistry;

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
		// When true, the RSSA presence-gating (auto-suppress an emulated Serial Adapter
		// on detecting a real one) is disabled. A safety opt-out: the detection can
		// false-positive on the controller's own status push to our emulated adapter.
		bool PresenceGatingDisabled{ false };

		// Expected equipment layout for the detected model (from PoolConfigurationDecoder);
		// 0 until the version page is scraped. Used to validate the discovered equipment set.
		uint8_t ExpectedAuxillaryCount{ 0 };
		uint8_t ExpectedPowerCenterCount{ 0 };

		// Outcome of cross-checking discovered equipment against the expected layout; set once
		// the startup scrape completes (std::nullopt until then).
		std::optional<Kernel::EquipmentValidation> EquipmentValidationResult;

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
	// SPA-SIDE SWITCH BUTTON ASSIGNMENTS
	//---------------------------------------------------------------------
	// The controller's spa-side switch button->function map, decoded from its config UI:
	// the iAQ "Spa Remotes" page or the OneTouch "Spa Switch" menu. Stored CONTROLLER-AGNOSTICALLY
	// (keyed by 1-based (switch, button)) so whichever controller a given system has populates the
	// same map -- the read path works for iAQ and OneTouch installs alike.

	public:
		void SetSpaSwitchAssignment(uint8_t switch_number, uint8_t button_number, const std::string& function);
		std::optional<std::string> SpaSwitchAssignment(uint8_t switch_number, uint8_t button_number) const;
		const std::map<std::pair<uint8_t, uint8_t>, std::string>& SpaSwitchAssignments() const;

	private:
		std::map<std::pair<uint8_t, uint8_t>, std::string> m_SpaSwitchAssignments;

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
		std::optional<Kernel::Temperature> PoolTempSetpoint2() const;
		std::optional<bool> PoolHeater2Enabled() const;
		std::optional<Kernel::Temperature> SpaTempSetpoint() const;
		std::optional<Kernel::Temperature> FreezeProtectPoint() const;
		Kernel::TemperatureUnits SystemTemperatureUnits() const;

	public:
		void AirTemp(const Kernel::Temperature& air_temp);
		void PoolTemp(const Kernel::Temperature& pool_temp);
		void SpaTemp(const Kernel::Temperature& spa_temp);
		void PoolTempSetpoint(const Kernel::Temperature& pool_temp_setpoint);
		void PoolTempSetpoint2(const Kernel::Temperature& pool_temp_setpoint_2);
		void PoolHeater2Enabled(bool pool_heater_2_enabled);
		void SpaTempSetpoint(const Kernel::Temperature& spa_temp_setpoint);
		void FreezeProtectPoint(const Kernel::Temperature& freeze_protect_point);
		void SystemTemperatureUnits(Kernel::TemperatureUnits units);

	private:
		std::optional<Kernel::Temperature> m_AirTemp;
		std::optional<Kernel::Temperature> m_PoolTemp;
		std::optional<Kernel::Temperature> m_SpaTemp;
		std::optional<Kernel::Temperature> m_PoolTempSetpoint;
		// Second pool setpoint -- POOLSP2, the panel's "TEMP2" maintenance (low) temperature. Only
		// present on single-body (Pool-Only/Spa-Only) systems, where TEMP1 (m_PoolTempSetpoint) is
		// the priority temperature and TEMP2 is a lower maintenance setpoint for the same body.
		std::optional<Kernel::Temperature> m_PoolTempSetpoint2;
		// Whether the panel's "TEMP2" maintenance heating (POOLHT2) is enabled. CAPTURE-GATED: the
		// underlying wire byte encoding is documented (boolean enable) but not yet live-validated.
		std::optional<bool> m_PoolHeater2Enabled;
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
		std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> DevicesOfType(AuxillaryTraitsTypes::AuxillaryTypes type) const;
		std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> Auxillaries() const;
		std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> Chlorinators() const;
		std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> Heaters() const;
		std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> Lights() const;
		std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> Pumps() const;
		std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> FilterPumps() const;

	public:
		// Count / existence predicates that avoid materialising a vector for the
		// common size() / empty() checks on the status hot path.
		uint32_t CountOfType(AuxillaryTraitsTypes::AuxillaryTypes type) const;
		bool HasAnyOfType(AuxillaryTraitsTypes::AuxillaryTypes type) const;
		uint32_t CountFilterPumps() const;
		bool HasAnyFilterPumps() const;

	public:
		[[deprecated("Use FilterPumps() instead; that returns a collection of pumps as there might be more than one")]]
		std::optional<std::shared_ptr<Kernel::AuxillaryDevice>> FilterPump();

	//---------------------------------------------------------------------
	// CONFIG-UPDATE EVENT EMITTERS
	//---------------------------------------------------------------------

	private:
		void EmitTemperatureEvent(const std::function<void(DataHub_ConfigEvent_Temperature&)>& populate) const;
		void EmitChemistryEvent(const std::function<void(DataHub_ConfigEvent_Chemistry&)>& populate) const;

	};

}
// namespace AqualinkAutomate::Equipment
