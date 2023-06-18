#pragma once

#include <nlohmann/json.hpp>

#include "jandy/config/jandy_config_auxillary.h"
#include "jandy/config/jandy_config_heater.h"
#include "jandy/config/jandy_config_pump.h"

namespace AqualinkAutomate::HTTP::JSON
{

	// NOTHING HERE

}
// namespace AqualinkAutomate::HTTP::JSON

namespace AqualinkAutomate::Config
{
	// Support the translation of the various DEVICE object types to JSON
	//
	// See https://github.com/nlohmann/json#arbitrary-types-conversions.

	void to_json(nlohmann::json& j, const Auxillary& device);
	void to_json(nlohmann::json& j, const Heater& device);
	void to_json(nlohmann::json& j, const Pump& device);

}
// namespace AqualinkAutomate::Config
