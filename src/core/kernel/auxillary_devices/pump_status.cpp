#include "kernel/auxillary_devices/pump_status.h"

namespace AqualinkAutomate::Kernel
{

	PumpStatuses ConvertToPumpStatus(Kernel::AuxillaryStatuses aux_status)
	{
		switch (aux_status)
		{
		case AuxillaryStatuses::On:
			return PumpStatuses::Running;

		case AuxillaryStatuses::Off:
			return PumpStatuses::Off;

		case AuxillaryStatuses::Enabled:
			[[fallthrough]];
		case AuxillaryStatuses::Pending:
			[[fallthrough]];
		case AuxillaryStatuses::Unknown:
			[[fallthrough]];
		default:
			return PumpStatuses::Unknown;
		}
	}

}
// namespace AqualinkAutomate::Kernel
