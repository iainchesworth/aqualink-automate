#pragma once

#include <optional>

#include <boost/uuid/uuid.hpp>
#include <nlohmann/json.hpp>

#include "kernel/data_hub_event.h"
#include "kernel/temperature.h"

namespace AqualinkAutomate::Kernel
{

	class DataHub_Event_Temperature : public DataHub_Event
	{
	public:
		DataHub_Event_Temperature();

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

	private:
		std::optional<Kernel::Temperature> m_PoolTemp;
		std::optional<Kernel::Temperature> m_SpaTemp;
		std::optional<Kernel::Temperature> m_AirTemp;
	};

	// Support the translation of the various WS event object types to JSON
	//
	// See https://github.com/nlohmann/json#arbitrary-types-conversions.

	void to_json(nlohmann::json& j, const DataHub_Event_Temperature& event);

}
// namespace namespace AqualinkAutomate::Kernel
