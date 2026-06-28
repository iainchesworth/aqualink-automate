#include <format>

#include <magic_enum/magic_enum.hpp>

#include "formatters/temperature_formatter.h"
#include "kernel/hub_events/data_hub_config_event_temperature.h"
#include "localisation/translations_and_units_formatter.h"

namespace AqualinkAutomate::Kernel
{

	DataHub_ConfigEvent_Temperature::DataHub_ConfigEvent_Temperature() :
		DataHub_ConfigEvent(Hub_EventTypes::Temperature),
		m_PoolTemp(std::nullopt),
		m_SpaTemp(std::nullopt),
		m_AirTemp(std::nullopt),
		m_PoolSetpoint(std::nullopt),
		m_PoolSetpoint2(std::nullopt),
		m_SpaSetpoint(std::nullopt)
	{
	}


	std::optional<Kernel::Temperature> DataHub_ConfigEvent_Temperature::PoolTemp() const
	{
		return m_PoolTemp;
	}

	std::optional<Kernel::Temperature> DataHub_ConfigEvent_Temperature::SpaTemp() const
	{
		return m_SpaTemp;
	}

	std::optional<Kernel::Temperature> DataHub_ConfigEvent_Temperature::AirTemp() const
	{
		return m_AirTemp;
	}

	void DataHub_ConfigEvent_Temperature::PoolTemp(const Kernel::Temperature& pool)
	{
		m_PoolTemp = pool;
	}

	void DataHub_ConfigEvent_Temperature::SpaTemp(const Kernel::Temperature& spa)
	{
		m_SpaTemp = spa;
	}

	void DataHub_ConfigEvent_Temperature::AirTemp(const Kernel::Temperature& air)
	{
		m_AirTemp = air;
	}

	std::optional<Kernel::Temperature> DataHub_ConfigEvent_Temperature::PoolSetpoint() const
	{
		return m_PoolSetpoint;
	}

	std::optional<Kernel::Temperature> DataHub_ConfigEvent_Temperature::SpaSetpoint() const
	{
		return m_SpaSetpoint;
	}

	void DataHub_ConfigEvent_Temperature::PoolSetpoint(const Kernel::Temperature& pool_setpoint)
	{
		m_PoolSetpoint = pool_setpoint;
	}

	std::optional<Kernel::Temperature> DataHub_ConfigEvent_Temperature::PoolSetpoint2() const
	{
		return m_PoolSetpoint2;
	}

	void DataHub_ConfigEvent_Temperature::PoolSetpoint2(const Kernel::Temperature& pool_setpoint_2)
	{
		m_PoolSetpoint2 = pool_setpoint_2;
	}

	void DataHub_ConfigEvent_Temperature::SpaSetpoint(const Kernel::Temperature& spa_setpoint)
	{
		m_SpaSetpoint = spa_setpoint;
	}

	nlohmann::json DataHub_ConfigEvent_Temperature::ToJSON() const
	{
		nlohmann::json j;

		if (m_PoolTemp.has_value())
		{
			j["pool_temp"] = Localisation::TranslationsAndUnitsFormatter::Instance().Localised(m_PoolTemp.value());
		}

		if (m_SpaTemp.has_value())
		{
			j["spa_temp"] = Localisation::TranslationsAndUnitsFormatter::Instance().Localised(m_SpaTemp.value());
		}

		if (m_AirTemp.has_value())
		{
			j["air_temp"] = Localisation::TranslationsAndUnitsFormatter::Instance().Localised(m_AirTemp.value());
		}

		if (m_PoolSetpoint.has_value())
		{
			j["pool_setpoint"] = {
				{"celsius", m_PoolSetpoint.value().InCelsius().value()},
				{"fahrenheit", m_PoolSetpoint.value().InFahrenheit().value()}
			};
		}

		if (m_PoolSetpoint2.has_value())
		{
			j["pool_setpoint_2"] = {
				{"celsius", m_PoolSetpoint2.value().InCelsius().value()},
				{"fahrenheit", m_PoolSetpoint2.value().InFahrenheit().value()}
			};
		}

		if (m_SpaSetpoint.has_value())
		{
			j["spa_setpoint"] = {
				{"celsius", m_SpaSetpoint.value().InCelsius().value()},
				{"fahrenheit", m_SpaSetpoint.value().InFahrenheit().value()}
			};
		}

		return j;
	}

}
// namespace AqualinkAutomate::Kernel
