#pragma once

#include <optional>

#include <boost/uuid/uuid.hpp>
#include <nlohmann/json.hpp>

#include "jandy/config/jandy_config_event.h"
#include "jandy/utility/string_conversion/temperature.h"

namespace AqualinkAutomate::Config
{

	class JandyConfig_Event_Temperature : public JandyConfig_Event
	{
	public:
		JandyConfig_Event_Temperature();

	public:
		std::optional<Utility::Temperature> PoolTemp() const;
		std::optional<Utility::Temperature> SpaTemp() const;
		std::optional<Utility::Temperature> AirTemp() const;

	public:
		void PoolTemp(Utility::Temperature pool);
		void SpaTemp(Utility::Temperature spa);
		void AirTemp(Utility::Temperature air);

	public:
		virtual boost::uuids::uuid Id() const override;

	private:
		std::optional<Utility::Temperature> m_PoolTemp;
		std::optional<Utility::Temperature> m_SpaTemp;
		std::optional<Utility::Temperature> m_AirTemp;
	};

	// Support the translation of the various WS event object types to JSON
	//
	// See https://github.com/nlohmann/json#arbitrary-types-conversions.

	void to_json(nlohmann::json& j, const JandyConfig_Event_Temperature& event);

}
// namespace namespace AqualinkAutomate::Config
