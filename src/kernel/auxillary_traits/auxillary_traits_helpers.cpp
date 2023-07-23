#include <magic_enum.hpp>

#include "kernel/auxillary_traits/auxillary_traits_helpers.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"

namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes
{

	std::string_view ConvertStatusToString(const AuxillaryDevice& device)
	{
		static const std::string UNKNOWN_STATUS{"Unknown"};

		if (!device.AuxillaryTraits.Has(AuxillaryTypeTrait{}))
		{
			// Cannot determine what type of device this is...
		}
		else if (!device.AuxillaryTraits.Has(StatusTrait{}))
		{
			// Cannot determine what the status of device this is...
		}
		else
		{
			switch (*(device.AuxillaryTraits[AuxillaryTypeTrait{}]))
			{
			case AuxillaryTypes::Auxillary:
				return magic_enum::enum_name(*(device.AuxillaryTraits[AuxillaryStatusTrait{}]));

			case AuxillaryTypes::Chlorinator:
				return magic_enum::enum_name(*(device.AuxillaryTraits[ChlorinatorStatusTrait{}]));

			case AuxillaryTypes::Heater:
				return magic_enum::enum_name(*(device.AuxillaryTraits[HeaterStatusTrait{}]));

			case AuxillaryTypes::Pump:
				return magic_enum::enum_name(*(device.AuxillaryTraits[PumpStatusTrait{}]));

			case AuxillaryTypes::Cleaner:
				[[fallthrough]];
			case AuxillaryTypes::Spillover:
				[[fallthrough]];
			case AuxillaryTypes::Sprinkler:
				[[fallthrough]];
			case AuxillaryTypes::Unknown:
				[[fallthrough]];
			default:
				// Unknown device type...
				break;
			}
		}

		return UNKNOWN_STATUS;
	}

	std::string_view ConvertStatusToString(std::shared_ptr<AuxillaryDevice> device)
	{
		static const std::string UNKNOWN_STATUS{"Unknown"};
		if (nullptr == device)
		{
			return UNKNOWN_STATUS;
		}

		return ConvertStatusToString(*device);
	}
}
// namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes
