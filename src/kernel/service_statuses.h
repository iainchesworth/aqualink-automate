#pragma once

namespace AqualinkAutomate::Kernel
{

	enum class ServiceStatuses
	{
		Initialising,
		Normal,
		LostCommsToEquipment,
		FaultOccurred,
		Unknown
	};

}
// namespace AqualinkAutomate::Kernel
