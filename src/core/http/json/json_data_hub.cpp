#include <format>

#include <boost/uuid/uuid_io.hpp>
#include <magic_enum/magic_enum.hpp>

#include "http/json/json_data_hub.h"
#include "kernel/auxillary_traits/auxillary_traits_helpers.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Kernel
{

	void to_json(nlohmann::json& j, const AuxillaryDevice& device)
	{
		const auto device_id_str = boost::uuids::to_string(device.Id());
		LogTrace(Channel::Web, [&device_id_str] { return std::format("Converting AuxillaryDevice {} to JSON", device_id_str); });

		nlohmann::json json_payload;

		json_payload["id"] = device_id_str;

		if (device.AuxillaryTraits.Has(AuxillaryTraitsTypes::LabelTrait{}))
		{
			const auto label = *(device.AuxillaryTraits[AuxillaryTraitsTypes::LabelTrait{}]);
			json_payload["label"] = label;
			LogTrace(Channel::Web, [&device_id_str, &label] { return std::format("  Device {} label: '{}'", device_id_str, label); });
		}
		else
		{
			LogTrace(Channel::Web, [&device_id_str] { return std::format("  Device {} has no label trait", device_id_str); });
		}

		const auto state = Kernel::AuxillaryTraitsTypes::ConvertStatusToString(device);
		json_payload["state"] = state;
		LogTrace(Channel::Web, [&device_id_str, &state] { return std::format("  Device {} state: {}", device_id_str, state); });

		if (device.AuxillaryTraits.Has(AuxillaryTraitsTypes::GeneratingPercentageTrait{}))
		{
			json_payload["generating_percentage"] = *(device.AuxillaryTraits[AuxillaryTraitsTypes::GeneratingPercentageTrait{}]);
		}

		if (device.AuxillaryTraits.Has(AuxillaryTraitsTypes::BoostModeTrait{}))
		{
			json_payload["boost_mode"] = std::string(magic_enum::enum_name(*(device.AuxillaryTraits[AuxillaryTraitsTypes::BoostModeTrait{}])));
		}

		if (device.AuxillaryTraits.Has(AuxillaryTraitsTypes::ChlorinatorHealthTrait{}))
		{
			json_payload["chlorinator_health"] = std::string(magic_enum::enum_name(*(device.AuxillaryTraits[AuxillaryTraitsTypes::ChlorinatorHealthTrait{}])));
		}

		if (device.AuxillaryTraits.Has(AuxillaryTraitsTypes::DutyCycleTrait{}))
		{
			json_payload["duty_cycle"] = *(device.AuxillaryTraits[AuxillaryTraitsTypes::DutyCycleTrait{}]);
		}

		if (device.AuxillaryTraits.Has(AuxillaryTraitsTypes::BodyOfWaterTrait{}))
		{
			json_payload["body_of_water"] = std::string(magic_enum::enum_name(*(device.AuxillaryTraits[AuxillaryTraitsTypes::BodyOfWaterTrait{}])));
		}

		j = json_payload;
	}

}
// namespace AqualinkAutomate::Kernel
