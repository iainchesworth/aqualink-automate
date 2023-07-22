#pragma once

#include <nlohmann/json.hpp>

#include "kernel/auxillary_devices/auxillary.h"
#include "kernel/auxillary_devices/chlorinator.h"
#include "kernel/auxillary_devices/heater.h"
#include "kernel/auxillary_devices/pump.h"

namespace AqualinkAutomate::HTTP::JSON
{

	// NOTHING HERE

}
// namespace AqualinkAutomate::HTTP::JSON

namespace AqualinkAutomate::Kernel
{
	// Support the translation of the various DEVICE object types to JSON
	//
	// See https://github.com/nlohmann/json#arbitrary-types-conversions.

	void to_json(nlohmann::json& j, const Auxillary& device);
	void to_json(nlohmann::json& j, const Chlorinator& device);
	void to_json(nlohmann::json& j, const Heater& device);
	void to_json(nlohmann::json& j, const Pump& device);

}
// namespace AqualinkAutomate::Kernel
