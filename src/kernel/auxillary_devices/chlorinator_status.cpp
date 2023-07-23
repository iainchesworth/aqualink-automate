#include "kernel/auxillary_devices/chlorinator_status.h"

namespace AqualinkAutomate::Kernel
{

	ChlorinatorStatuses ConvertToChlorinatorStatus(Kernel::AuxillaryStatuses aux_status)
	{
		switch (aux_status)
		{
		case AuxillaryStatuses::On:
			return ChlorinatorStatuses::Running;

		case AuxillaryStatuses::Off:
			return ChlorinatorStatuses::Off;

		case AuxillaryStatuses::Enabled:
			[[fallthrough]];
		case AuxillaryStatuses::Pending:
			[[fallthrough]];
		case AuxillaryStatuses::Unknown:
			[[fallthrough]];
		default:
			return ChlorinatorStatuses::Unknown;
		}
	}

}
// namespace AqualinkAutomate::Kernel
