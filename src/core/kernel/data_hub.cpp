#include <algorithm>
#include <execution>

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

	Kernel::Temperature DataHub::AirTemp() const
	{
		return m_AirTemp;
	}

	Kernel::Temperature DataHub::PoolTemp() const
	{
		return m_PoolTemp;
	}

	Kernel::Temperature DataHub::SpaTemp() const
	{
		return m_SpaTemp;
	}

	Kernel::Temperature DataHub::FreezeProtectPoint() const
	{
		return m_FreezeProtectPoint;
	}

	void DataHub::AirTemp(const Kernel::Temperature& air_temp)
	{
		m_AirTemp = air_temp;
		Factory::ProfilerFactory::Instance().Get()->PlotValue("Air Temp", m_AirTemp.InCelsius().value());

		// Signal that a temperature update has occurred.
		auto update_event = std::make_shared<DataHub_ConfigEvent_Temperature>();
		update_event->AirTemp(m_AirTemp);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::PoolTemp(const Kernel::Temperature& pool_temp)
	{
		m_PoolTemp = pool_temp;
		Factory::ProfilerFactory::Instance().Get()->PlotValue("Pool Temp", m_PoolTemp.InCelsius().value());

		// Signal that a temperature update has occurred.
		auto update_event = std::make_shared<DataHub_ConfigEvent_Temperature>();
		update_event->PoolTemp(m_PoolTemp);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::SpaTemp(const Kernel::Temperature& spa_temp)
	{
		m_SpaTemp = spa_temp;
		Factory::ProfilerFactory::Instance().Get()->PlotValue("Spa Temp", m_SpaTemp.InCelsius().value());

		// Signal that a temperature update has occurred.
		auto update_event = std::make_shared<DataHub_ConfigEvent_Temperature>();
		update_event->SpaTemp(m_SpaTemp);
		ConfigUpdateSignal(update_event);
	}

	Kernel::Temperature DataHub::PoolTempSetpoint() const
	{
		return m_PoolTempSetpoint;
	}

	Kernel::Temperature DataHub::SpaTempSetpoint() const
	{
		return m_SpaTempSetpoint;
	}

	void DataHub::PoolTempSetpoint(const Kernel::Temperature& pool_temp_setpoint)
	{
		m_PoolTempSetpoint = pool_temp_setpoint;
		Factory::ProfilerFactory::Instance().Get()->PlotValue("Pool Temp Setpoint", m_PoolTempSetpoint.InCelsius().value());

		// Signal that a temperature update has occurred.
		auto update_event = std::make_shared<DataHub_ConfigEvent_Temperature>();
		update_event->PoolSetpoint(m_PoolTempSetpoint);
		ConfigUpdateSignal(update_event);
	}

	void DataHub::SpaTempSetpoint(const Kernel::Temperature& spa_temp_setpoint)
	{
		m_SpaTempSetpoint = spa_temp_setpoint;
		Factory::ProfilerFactory::Instance().Get()->PlotValue("Spa Temp Setpoint", m_SpaTempSetpoint.InCelsius().value());

		// Signal that a temperature update has occurred.
		auto update_event = std::make_shared<DataHub_ConfigEvent_Temperature>();
		update_event->SpaSetpoint(m_SpaTempSetpoint);
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
		if (nullptr != m_CachedFilterPump)
		{
			// Already know which device is the filter pump so use the cache.
			return m_CachedFilterPump;
		}
		else
		{
			for (const auto& base_ptr : Devices.FindByLabel(Kernel::AuxillaryTraitsTypes::LabelTrait::COMMON_LABEL_FILTER_PUMP))
			{
				if (nullptr != base_ptr)
				{
					// If there's at least one matching pump, use the first one found...
					m_CachedFilterPump = base_ptr;
					return m_CachedFilterPump;
				}
			}
		}

		return std::nullopt;
	}

}
// namespace AqualinkAutomate::Kernel
