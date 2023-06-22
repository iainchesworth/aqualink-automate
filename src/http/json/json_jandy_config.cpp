#include <boost/uuid/uuid_io.hpp>
#include <magic_enum.hpp>

#include "http/json/json_jandy_config.h"

namespace AqualinkAutomate::HTTP::JSON
{

	// NOTHING HERE

}
// namespace AqualinkAutomate::HTTP::JSON

namespace AqualinkAutomate::Config
{

	void to_json(nlohmann::json& j, const Auxillary& device)
	{
		j = nlohmann::json{ 
			{"id", boost::uuids::to_string(device.Id())},
			{"label", device.Label()}, 
			{"state", magic_enum::enum_name(device.State())} 
		};
	}

	void to_json(nlohmann::json& j, const Heater& device)
	{
		j = nlohmann::json{ 
			{"id", boost::uuids::to_string(device.Id())},
			{"label", device.Label()}, 
			{"state", magic_enum::enum_name(device.State())}
		};
	}

	void to_json(nlohmann::json& j, const Pump& device)
	{
		j = nlohmann::json{ 
			{"id", boost::uuids::to_string(device.Id())},
			{"label", device.Label()}, 
			{"state", magic_enum::enum_name(device.State())}
		};
	}

}
// namespace AqualinkAutomate::Config