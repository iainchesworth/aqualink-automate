#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/data_hub.h"
#include "kernel/hub_events/data_hub_config_event_chemistry.h"
#include "kernel/hub_events/data_hub_config_event_temperature.h"
#include "kernel/hub_events/equipment_hub_system_event_status_change.h"
#include "profiling/factories/profiler_factory.h"
#include "utility/case_insensitive_comparision.h"

namespace AqualinkAutomate::Kernel
{
	
	DataHub::DataHub() :
		IHub()
	{
	}

	void DataHub::ApplyPoolConfiguration(PoolConfigurations config, ConfigurationSource source)
	{
		PoolConfiguration = config;
		PoolConfigurationSource = source;

		if (!m_Bodies.empty())
		{
			return; // Bodies already populated
		}

		switch (config)
		{
		case PoolConfigurations::DualBody_SharedEquipment:
		case PoolConfigurations::DualBody_DualEquipment:
			AddBody(BodyOfWater{ BodyOfWaterIds::Pool, "Pool" });
			AddBody(BodyOfWater{ BodyOfWaterIds::Spa, "Spa" });
			break;
		case PoolConfigurations::SingleBody:
			AddBody(BodyOfWater{ BodyOfWaterIds::Pool, "Pool" });
			break;
		default:
			break;
		}

		// Set active body based on current circulation mode.
		// The Jandy controller always has one body active (Pool by default).
		bool spa_active = (CirculationMode == CirculationModes::Spa
			|| CirculationMode == CirculationModes::SpaFill
			|| CirculationMode == CirculationModes::SpaDrain);

		if (auto pool = GetBody(BodyOfWaterIds::Pool))
		{
			pool->get().IsActive(!spa_active);
		}

		if (auto spa = GetBody(BodyOfWaterIds::Spa))
		{
			spa->get().IsActive(spa_active);
		}
	}

	void DataHub::AddBody(BodyOfWater body)
	{
		// Avoid duplicates
		for (const auto& existing : m_Bodies)
		{
			if (existing.Id() == body.Id())
			{
				return;
			}
		}

		m_Bodies.push_back(std::move(body));
	}

	std::optional<std::reference_wrapper<BodyOfWater>> DataHub::GetBody(BodyOfWaterIds id)
	{
		for (auto& body : m_Bodies)
		{
			if (body.Id() == id)
			{
				return body;
			}
		}

		return std::nullopt;
	}

	std::optional<std::reference_wrapper<const BodyOfWater>> DataHub::GetBody(BodyOfWaterIds id) const
	{
		for (const auto& body : m_Bodies)
		{
			if (body.Id() == id)
			{
				return body;
			}
		}

		return std::nullopt;
	}

	std::optional<std::reference_wrapper<BodyOfWater>> DataHub::ActiveBody()
	{
		for (auto& body : m_Bodies)
		{
			if (body.IsActive())
			{
				return body;
			}
		}

		return std::nullopt;
	}

	const std::vector<BodyOfWater>& DataHub::Bodies() const
	{
		return m_Bodies;
	}

	std::optional<Kernel::Temperature> DataHub::AirTemp() const
	{
		return m_AirTemp;
	}

	std::optional<Kernel::Temperature> DataHub::PoolTemp() const
	{
		if (auto body = GetBody(BodyOfWaterIds::Pool))
		{
			return body->get().CurrentTemp();
		}

		return m_PoolTemp;
	}

	std::optional<Kernel::Temperature> DataHub::SpaTemp() const
	{
		if (auto body = GetBody(BodyOfWaterIds::Spa))
		{
			return body->get().CurrentTemp();
		}

		return m_SpaTemp;
	}

	std::optional<Kernel::Temperature> DataHub::FreezeProtectPoint() const
	{
		return m_FreezeProtectPoint;
	}

	void DataHub::AirTemp(const Kernel::Temperature& air_temp)
	{
		m_AirTemp = air_temp;
		Factory::ProfilerFactory::Instance().Get()->PlotValue("Air Temp", air_temp.InCelsius().value());

		// Signal that a temperature update has occurred.
		auto update_event = std::make_shared<DataHub_ConfigEvent_Temperature>();
		update_event->AirTemp(air_temp);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::PoolTemp(const Kernel::Temperature& pool_temp)
	{
		m_PoolTemp = pool_temp;

		if (auto body = GetBody(BodyOfWaterIds::Pool))
		{
			body->get().CurrentTemp(pool_temp);
		}

		Factory::ProfilerFactory::Instance().Get()->PlotValue("Pool Temp", pool_temp.InCelsius().value());

		// Signal that a temperature update has occurred.
		auto update_event = std::make_shared<DataHub_ConfigEvent_Temperature>();
		update_event->PoolTemp(pool_temp);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::SpaTemp(const Kernel::Temperature& spa_temp)
	{
		m_SpaTemp = spa_temp;

		if (auto body = GetBody(BodyOfWaterIds::Spa))
		{
			body->get().CurrentTemp(spa_temp);
		}

		Factory::ProfilerFactory::Instance().Get()->PlotValue("Spa Temp", spa_temp.InCelsius().value());

		// Signal that a temperature update has occurred.
		auto update_event = std::make_shared<DataHub_ConfigEvent_Temperature>();
		update_event->SpaTemp(spa_temp);
		ConfigUpdateSignal(update_event);
	}

	std::optional<Kernel::Temperature> DataHub::PoolTempSetpoint() const
	{
		if (auto body = GetBody(BodyOfWaterIds::Pool))
		{
			return body->get().TempSetpoint();
		}

		return m_PoolTempSetpoint;
	}

	std::optional<Kernel::Temperature> DataHub::SpaTempSetpoint() const
	{
		if (auto body = GetBody(BodyOfWaterIds::Spa))
		{
			return body->get().TempSetpoint();
		}

		return m_SpaTempSetpoint;
	}

	void DataHub::PoolTempSetpoint(const Kernel::Temperature& pool_temp_setpoint)
	{
		m_PoolTempSetpoint = pool_temp_setpoint;

		if (auto body = GetBody(BodyOfWaterIds::Pool))
		{
			body->get().TempSetpoint(pool_temp_setpoint);
		}

		Factory::ProfilerFactory::Instance().Get()->PlotValue("Pool Temp Setpoint", pool_temp_setpoint.InCelsius().value());

		// Signal that a temperature update has occurred.
		auto update_event = std::make_shared<DataHub_ConfigEvent_Temperature>();
		update_event->PoolSetpoint(pool_temp_setpoint);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::SpaTempSetpoint(const Kernel::Temperature& spa_temp_setpoint)
	{
		m_SpaTempSetpoint = spa_temp_setpoint;

		if (auto body = GetBody(BodyOfWaterIds::Spa))
		{
			body->get().TempSetpoint(spa_temp_setpoint);
		}

		Factory::ProfilerFactory::Instance().Get()->PlotValue("Spa Temp Setpoint", spa_temp_setpoint.InCelsius().value());

		// Signal that a temperature update has occurred.
		auto update_event = std::make_shared<DataHub_ConfigEvent_Temperature>();
		update_event->SpaSetpoint(spa_temp_setpoint);
		ConfigUpdateSignal(update_event);
	}

	Kernel::TemperatureUnits DataHub::SystemTemperatureUnits() const
	{
		return m_SystemTemperatureUnits;
	}

	void DataHub::SystemTemperatureUnits(Kernel::TemperatureUnits units)
	{
		m_SystemTemperatureUnits = units;
	}

	void DataHub::FreezeProtectPoint(const Kernel::Temperature& freeze_protect_point)
	{
		m_FreezeProtectPoint = freeze_protect_point;
	}

	Kernel::ORP DataHub::ORP() const
	{
		return m_ORP;
	}

	Kernel::pH DataHub::pH() const
	{
		return m_pH;
	}

	ppm_quantity DataHub::SaltLevel() const
	{
		return m_SaltLevel;
	}

	void DataHub::ORP(const Kernel::ORP& orp)
	{
		m_ORP = orp;

		// Signal that a chemistry update has occurred.
		auto update_event = std::make_shared<DataHub_ConfigEvent_Chemistry>();
		update_event->ORP(m_ORP);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::pH(const Kernel::pH& pH)
	{
		m_pH = pH;

		// Signal that a chemistry update has occurred.
		auto update_event = std::make_shared<DataHub_ConfigEvent_Chemistry>();
		update_event->pH(m_pH);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::SaltLevel(const ppm_quantity& salt_level_in_ppm)
	{
		m_SaltLevel = salt_level_in_ppm;

		// Signal that a chemistry update has occurred.
		auto update_event = std::make_shared<DataHub_ConfigEvent_Chemistry>();
		update_event->SaltLevel(m_SaltLevel);
		ConfigUpdateSignal(update_event);
	}

	std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> DataHub::Auxillaries() const
	{
		using DeviceType = decltype(Auxillaries())::value_type::element_type;
		return Devices.FindByTrait(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Auxillary);
	}

	std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> DataHub::Chlorinators() const
	{
		using DeviceType = decltype(Chlorinators())::value_type::element_type;
		return Devices.FindByTrait(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Chlorinator);
	}

	std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> DataHub::Heaters() const
	{
		using DeviceType = decltype(Heaters())::value_type::element_type;
		return Devices.FindByTrait(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Heater);
	}

	std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> DataHub::Pumps() const
	{
		using DeviceType = decltype(Pumps())::value_type::element_type;
		return Devices.FindByTrait(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Pump);
	}

	std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> DataHub::FilterPumps() const
	{
		using DeviceType = decltype(Pumps())::value_type::element_type;
		return Devices.FindByTrait(AuxillaryTraitsTypes::PumpTypeTrait{}, PumpTypes::FilterCirculation);
	}

	std::optional<std::shared_ptr<Kernel::AuxillaryDevice>> DataHub::FilterPump()
	{
		auto pumps = FilterPumps();
		if (pumps.empty())
		{
			return std::nullopt;
		}
		return pumps.front();
	}

}
// namespace AqualinkAutomate::Kernel
