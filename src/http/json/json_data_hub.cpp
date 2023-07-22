#include <boost/uuid/uuid_io.hpp>
#include <magic_enum.hpp>

#include "http/json/json_data_hub.h"

namespace AqualinkAutomate::HTTP::JSON
{

	// NOTHING HERE

}
// namespace AqualinkAutomate::HTTP::JSON

namespace AqualinkAutomate::Kernel
{

	void to_json(nlohmann::json& j, const Auxillary& device)
	{
		j = nlohmann::json{ 
			{"id", boost::uuids::to_string(device.Id())},
			{"label", device.Label()}, 
			{"state", magic_enum::enum_name(device.Status())} 
		};
	}

	void to_json(nlohmann::json& j, const Chlorinator& device)
	{
		j = nlohmann::json{
			{"id", boost::uuids::to_string(device.Id())},
			{"label", device.Label()},
			{"state", magic_enum::enum_name(device.Status())}
		};
	}

	void to_json(nlohmann::json& j, const Heater& device)
	{
		j = nlohmann::json{ 
			{"id", boost::uuids::to_string(device.Id())},
			{"label", device.Label()}, 
			{"state", magic_enum::enum_name(device.Status())}
		};
	}

	void to_json(nlohmann::json& j, const Pump& device)
	{
		j = nlohmann::json{ 
			{"id", boost::uuids::to_string(device.Id())},
			{"label", device.Label()}, 
			{"state", magic_enum::enum_name(device.Status())}
		};
	}

}
// namespace AqualinkAutomate::Kernel
