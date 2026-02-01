#include <boost/uuid/uuid_io.hpp>
#include <magic_enum/magic_enum.hpp>

#include "http/json/json_data_hub.h"
#include "kernel/auxillary_traits/auxillary_traits_helpers.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP::JSON
{

	// NOTHING HERE

}
// namespace AqualinkAutomate::HTTP::JSON

namespace AqualinkAutomate::Kernel
{

	void to_json(nlohmann::json& j, const AuxillaryDevice& device)
	{
		using namespace AqualinkAutomate::Logging;

		const auto device_id_str = boost::uuids::to_string(device.Id());
		LogTrace(Channel::Web, std::format("Converting AuxillaryDevice {} to JSON", device_id_str));

		nlohmann::json json_payload;

		json_payload["id"] = device_id_str;

		if (device.AuxillaryTraits.Has(AuxillaryTraitsTypes::LabelTrait{}))
		{
			const auto label = *(device.AuxillaryTraits[AuxillaryTraitsTypes::LabelTrait{}]);
			json_payload["label"] = label;
			LogTrace(Channel::Web, std::format("  Device {} label: '{}'", device_id_str, label));
		}
		else
		{
			LogTrace(Channel::Web, std::format("  Device {} has no label trait", device_id_str));
		}

		const auto state = Kernel::AuxillaryTraitsTypes::ConvertStatusToString(device);
		json_payload["state"] = state;
		LogTrace(Channel::Web, std::format("  Device {} state: {}", device_id_str, state));

		j = json_payload;
	}

}
// namespace AqualinkAutomate::Kernel
