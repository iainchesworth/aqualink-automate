#include <boost/uuid/uuid_io.hpp>
#include <magic_enum.hpp>

#include "http/json/json_data_hub.h"
#include "kernel/auxillary_traits/auxillary_traits_helpers.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"

namespace AqualinkAutomate::HTTP::JSON
{

	// NOTHING HERE

}
// namespace AqualinkAutomate::HTTP::JSON

namespace AqualinkAutomate::Kernel
{

	void to_json(nlohmann::json& j, const AuxillaryDevice& device)
	{
		nlohmann::json json_payload;

		json_payload["id"] = boost::uuids::to_string(device.Id());

		if (device.AuxillaryTraits.Has(AuxillaryTraitsTypes::LabelTrait{}))
		{
			json_payload["label"] = *(device.AuxillaryTraits[AuxillaryTraitsTypes::LabelTrait{}]);
		}

		json_payload["state"] = Kernel::AuxillaryTraitsTypes::ConvertStatusToString(device);

		j = json_payload;
	}

}
// namespace AqualinkAutomate::Kernel
