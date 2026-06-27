#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/data_hub.h"
#include "kernel/hub_events/data_hub_config_event_chemistry.h"
#include "kernel/hub_events/data_hub_config_event_temperature.h"
#include "kernel/hub_events/equipment_hub_system_event_status_change.h"
#include "profiling/factories/profiler_factory.h"
#include "profiling/factories/profiling_unit_factory.h"
#include "utility/case_insensitive_comparision.h"

#include <source_location>

namespace AqualinkAutomate::Kernel
{
	
	DataHub::DataHub() = default;

	void DataHub::EmitTemperatureEvent(const std::function<void(DataHub_ConfigEvent_Temperature&)>& populate) const
	{
		// Zone the synchronous signals2 fan-out: this measures the cost of every
		// registered listener (WebSocket push, MQTT publish, etc.) reacting to a
		// temperature update.
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("DataHub::EmitTemperatureEvent", std::source_location::current());

		// Signal that a temperature update has occurred.
		auto update_event = std::make_shared<DataHub_ConfigEvent_Temperature>();
		populate(*update_event);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::EmitChemistryEvent(const std::function<void(DataHub_ConfigEvent_Chemistry&)>& populate) const
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("DataHub::EmitChemistryEvent", std::source_location::current());

		// Signal that a chemistry update has occurred.
		auto update_event = std::make_shared<DataHub_ConfigEvent_Chemistry>();
		populate(*update_event);
		ConfigUpdateSignal(update_event);
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

		EmitTemperatureEvent([&air_temp](DataHub_ConfigEvent_Temperature& update_event)
			{
				update_event.AirTemp(air_temp);
			});
	}

	void DataHub::PoolTemp(const Kernel::Temperature& pool_temp)
	{
		m_PoolTemp = pool_temp;

		if (auto body = GetBody(BodyOfWaterIds::Pool))
		{
			body->get().CurrentTemp(pool_temp);
		}

		Factory::ProfilerFactory::Instance().Get()->PlotValue("Pool Temp", pool_temp.InCelsius().value());

		EmitTemperatureEvent([&pool_temp](DataHub_ConfigEvent_Temperature& update_event)
			{
				update_event.PoolTemp(pool_temp);
			});
	}

	void DataHub::SpaTemp(const Kernel::Temperature& spa_temp)
	{
		m_SpaTemp = spa_temp;

		if (auto body = GetBody(BodyOfWaterIds::Spa))
		{
			body->get().CurrentTemp(spa_temp);
		}

		Factory::ProfilerFactory::Instance().Get()->PlotValue("Spa Temp", spa_temp.InCelsius().value());

		EmitTemperatureEvent([&spa_temp](DataHub_ConfigEvent_Temperature& update_event)
			{
				update_event.SpaTemp(spa_temp);
			});
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

		EmitTemperatureEvent([&pool_temp_setpoint](DataHub_ConfigEvent_Temperature& update_event)
			{
				update_event.PoolSetpoint(pool_temp_setpoint);
			});
	}

	void DataHub::SpaTempSetpoint(const Kernel::Temperature& spa_temp_setpoint)
	{
		m_SpaTempSetpoint = spa_temp_setpoint;

		if (auto body = GetBody(BodyOfWaterIds::Spa))
		{
			body->get().TempSetpoint(spa_temp_setpoint);
		}

		Factory::ProfilerFactory::Instance().Get()->PlotValue("Spa Temp Setpoint", spa_temp_setpoint.InCelsius().value());

		EmitTemperatureEvent([&spa_temp_setpoint](DataHub_ConfigEvent_Temperature& update_event)
			{
				update_event.SpaSetpoint(spa_temp_setpoint);
			});
	}

	Kernel::TemperatureUnits DataHub::SystemTemperatureUnits() const
	{
		return m_SystemTemperatureUnits;
	}

	void DataHub::SystemTemperatureUnits(Kernel::TemperatureUnits units)
	{
		m_SystemTemperatureUnits = units;

		// Deliberately does NOT emit a ConfigUpdateSignal: DataHub_ConfigEvent_Temperature
		// carries no temperature-units field, so an emitted event would be an empty
		// ({}) no-op for WebSocket/MQTT consumers. If a units change ever needs to be
		// surfaced, add a units field to the temperature event (owned by the hub-events
		// unit) and emit it here via EmitTemperatureEvent.
	}

	void DataHub::FreezeProtectPoint(const Kernel::Temperature& freeze_protect_point)
	{
		m_FreezeProtectPoint = freeze_protect_point;

		// Deliberately does NOT emit a ConfigUpdateSignal: DataHub_ConfigEvent_Temperature
		// carries no freeze-protect field, so an emitted event would be an empty ({})
		// no-op for WebSocket/MQTT consumers. If the freeze-protect setpoint ever needs
		// to be surfaced, add a field to the temperature event (owned by the hub-events
		// unit) and emit it here via EmitTemperatureEvent.
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

		EmitChemistryEvent([this](DataHub_ConfigEvent_Chemistry& update_event)
			{
				update_event.ORP(m_ORP);
			});
	}

	void DataHub::pH(const Kernel::pH& pH)
	{
		m_pH = pH;

		EmitChemistryEvent([this](DataHub_ConfigEvent_Chemistry& update_event)
			{
				update_event.pH(m_pH);
			});
	}

	void DataHub::SaltLevel(const ppm_quantity& salt_level_in_ppm)
	{
		m_SaltLevel = salt_level_in_ppm;

		EmitChemistryEvent([this](DataHub_ConfigEvent_Chemistry& update_event)
			{
				update_event.SaltLevel(m_SaltLevel);
			});
	}

	std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> DataHub::DevicesOfType(AuxillaryTraitsTypes::AuxillaryTypes type) const
	{
		return Devices.FindByTrait(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, type);
	}

	std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> DataHub::Auxillaries() const
	{
		return DevicesOfType(AuxillaryTraitsTypes::AuxillaryTypes::Auxillary);
	}

	std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> DataHub::Chlorinators() const
	{
		return DevicesOfType(AuxillaryTraitsTypes::AuxillaryTypes::Chlorinator);
	}

	std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> DataHub::Heaters() const
	{
		return DevicesOfType(AuxillaryTraitsTypes::AuxillaryTypes::Heater);
	}

	std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> DataHub::Lights() const
	{
		return DevicesOfType(AuxillaryTraitsTypes::AuxillaryTypes::Light);
	}

	std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> DataHub::Pumps() const
	{
		return DevicesOfType(AuxillaryTraitsTypes::AuxillaryTypes::Pump);
	}

	std::vector<std::shared_ptr<Kernel::AuxillaryDevice>> DataHub::FilterPumps() const
	{
		return Devices.FindByTrait(AuxillaryTraitsTypes::PumpTypeTrait{}, PumpTypes::FilterCirculation);
	}

	uint32_t DataHub::CountOfType(AuxillaryTraitsTypes::AuxillaryTypes type) const
	{
		return Devices.CountByTrait(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, type);
	}

	bool DataHub::HasAnyOfType(AuxillaryTraitsTypes::AuxillaryTypes type) const
	{
		return Devices.HasAnyByTrait(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, type);
	}

	uint32_t DataHub::CountFilterPumps() const
	{
		return Devices.CountByTrait(AuxillaryTraitsTypes::PumpTypeTrait{}, PumpTypes::FilterCirculation);
	}

	bool DataHub::HasAnyFilterPumps() const
	{
		return Devices.HasAnyByTrait(AuxillaryTraitsTypes::PumpTypeTrait{}, PumpTypes::FilterCirculation);
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

	void DataHub::SetSpaSwitchAssignment(uint8_t switch_number, uint8_t button_number, const std::string& function)
	{
		m_SpaSwitchAssignments[std::make_pair(switch_number, button_number)] = function;
	}

	std::optional<std::string> DataHub::SpaSwitchAssignment(uint8_t switch_number, uint8_t button_number) const
	{
		const auto it = m_SpaSwitchAssignments.find(std::make_pair(switch_number, button_number));
		if (it == m_SpaSwitchAssignments.end())
		{
			return std::nullopt;
		}
		return it->second;
	}

	const std::map<std::pair<uint8_t, uint8_t>, std::string>& DataHub::SpaSwitchAssignments() const
	{
		return m_SpaSwitchAssignments;
	}

}
// namespace AqualinkAutomate::Kernel
