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
		~DataHub_ConfigEvent_Temperature();

	public:
		std::optional<Kernel::Temperature> PoolTemp() const;
		std::optional<Kernel::Temperature> SpaTemp() const;
		std::optional<Kernel::Temperature> AirTemp() const;

	public:
		void PoolTemp(Kernel::Temperature pool);
		void SpaTemp(Kernel::Temperature spa);
		void AirTemp(Kernel::Temperature air);

	public:
		virtual boost::uuids::uuid Id() const override;
		virtual nlohmann::json ToJSON() const override;

	private:
		std::optional<Kernel::Temperature> m_PoolTemp;
		std::optional<Kernel::Temperature> m_SpaTemp;
		std::optional<Kernel::Temperature> m_AirTemp;
	};

}
// namespace AqualinkAutomate::Kernel
