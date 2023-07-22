#include <magic_enum.hpp>

#include "kernel/auxillary_traits/auxillary_traits_helpers.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"

namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes
{

	std::string_view ConvertStatusToString(std::shared_ptr<AuxillaryBase> device)
	{
		static const std::string UNKNOWN_STATUS{"Unknown"};

		if (nullptr == device)
		{
			// Invalid device pointer....return unknown status.
		}
		else if (!device->AuxillaryTraits.Has(AuxillaryTypeTrait{}))
		{
			// Cannot determine what type of device this is...
		}
		else if (!device->AuxillaryTraits.Has(StatusTrait{}))
		{
			// Cannot determine what the status of device this is...
		}
		else
		{
			switch (*(device->AuxillaryTraits[AuxillaryTypeTrait{}]))
			{
			case AuxillaryTypes::Auxillary:
				return magic_enum::enum_name(*(device->AuxillaryTraits[AuxillaryStatusTrait{}]));

			case AuxillaryTypes::Chlorinator:
				return magic_enum::enum_name(*(device->AuxillaryTraits[ChlorinatorStatusTrait{}]));

			case AuxillaryTypes::Heater:
				return magic_enum::enum_name(*(device->AuxillaryTraits[HeaterStatusTrait{}]));

			case AuxillaryTypes::Pump:
				return magic_enum::enum_name(*(device->AuxillaryTraits[PumpStatusTrait{}]));

			default:
				// Unknown device type...
				break;
			}
		}

		return UNKNOWN_STATUS;
	}

}
// namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes
