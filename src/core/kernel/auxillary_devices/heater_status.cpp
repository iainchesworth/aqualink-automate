#include "kernel/auxillary_devices/heater_status.h"

namespace AqualinkAutomate::Kernel
{

	HeaterStatuses ConvertToHeaterStatus(Kernel::AuxillaryStatuses aux_status)
	{
		switch (aux_status)
		{
		case AuxillaryStatuses::On:
			return HeaterStatuses::Heating;

		case AuxillaryStatuses::Off:
			return HeaterStatuses::Off;

		case AuxillaryStatuses::Enabled:
			return HeaterStatuses::Enabled;

		case AuxillaryStatuses::Pending:
			[[fallthrough]];
		case AuxillaryStatuses::Unknown:
			[[fallthrough]];
		default:
			return HeaterStatuses::Unknown;
		}
	}

}
// namespace AqualinkAutomate::Kernel
