#include <format>

#include <boost/uuid/uuid_generators.hpp>
#include <magic_enum.hpp>

#include "formatters/temperature_formatter.h"
#include "kernel/data_hub_event_temperature.h"

namespace AqualinkAutomate::Kernel
{

	DataHub_Event_Temperature::DataHub_Event_Temperature() :
		DataHub_Event(DataHub_EventTypes::Temperature),
		m_PoolTemp(std::nullopt),
		m_SpaTemp(std::nullopt),
		m_AirTemp(std::nullopt)
	{
	}

	std::optional<Kernel::Temperature> DataHub_Event_Temperature::PoolTemp() const
	{
		return m_PoolTemp;
	}

	std::optional<Kernel::Temperature> DataHub_Event_Temperature::SpaTemp() const
	{
		return m_SpaTemp;
	}

	std::optional<Kernel::Temperature> DataHub_Event_Temperature::AirTemp() const
	{
		return m_AirTemp;
	}

	void DataHub_Event_Temperature::PoolTemp(Kernel::Temperature pool)
	{
		m_PoolTemp = pool;
	}

	void DataHub_Event_Temperature::SpaTemp(Kernel::Temperature spa)
	{
		m_SpaTemp = spa;
	}

	void DataHub_Event_Temperature::AirTemp(Kernel::Temperature air)
	{
		m_AirTemp = air;
	}

	boost::uuids::uuid DataHub_Event_Temperature::Id() const
	{
		static boost::uuids::uuid id{ boost::uuids::random_generator()() };
		return id;
	}

	void to_json(nlohmann::json& j, const DataHub_Event_Temperature& event)
	{
		if (event.PoolTemp().has_value())
		{
			j["pool_temp"] = std::format("{}", event.PoolTemp().value());
		}

		if (event.SpaTemp().has_value())
		{
			j["spa_temp"] = std::format("{}", event.SpaTemp().value());
		}

		if (event.AirTemp().has_value())
		{
			j["air_temp"] = std::format("{}", event.AirTemp().value());
		}
	}

}
// namespace AqualinkAutomate::Kernel
