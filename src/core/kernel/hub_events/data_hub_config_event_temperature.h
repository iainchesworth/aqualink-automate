#pragma once

#include <optional>

#include <boost/uuid/uuid.hpp>
#include <nlohmann/json.hpp>

#include "kernel/temperature.h"
#include "kernel/hub_events/data_hub_config_event.h"

namespace AqualinkAutomate::Kernel
{

	class DataHub_ConfigEvent_Temperature : public DataHub_ConfigEvent
	{
	public:
		DataHub_ConfigEvent_Temperature();
		~DataHub_ConfigEvent_Temperature() override = default;

	public:
		std::optional<Kernel::Temperature> PoolTemp() const;
		std::optional<Kernel::Temperature> SpaTemp() const;
		std::optional<Kernel::Temperature> AirTemp() const;
		std::optional<Kernel::Temperature> PoolSetpoint() const;
		std::optional<Kernel::Temperature> SpaSetpoint() const;

	public:
		void PoolTemp(const Kernel::Temperature& pool);
		void SpaTemp(const Kernel::Temperature& spa);
		void AirTemp(const Kernel::Temperature& air);
		void PoolSetpoint(const Kernel::Temperature& pool_setpoint);
		void SpaSetpoint(const Kernel::Temperature& spa_setpoint);

	public:
		boost::uuids::uuid Id() const override;
		nlohmann::json ToJSON() const override;

	private:
		std::optional<Kernel::Temperature> m_PoolTemp;
		std::optional<Kernel::Temperature> m_SpaTemp;
		std::optional<Kernel::Temperature> m_AirTemp;
		std::optional<Kernel::Temperature> m_PoolSetpoint;
		std::optional<Kernel::Temperature> m_SpaSetpoint;
	};

}
// namespace AqualinkAutomate::Kernel
