#include <typeinfo>

#include "interfaces/idevicestatus.h"

namespace AqualinkAutomate::Interfaces
{

	bool operator==(const IDeviceStatus& lhs, const IDeviceStatus& rhs)
	{
		return (typeid(lhs) == typeid(rhs));
	}

	bool operator!=(const IDeviceStatus& lhs, const IDeviceStatus& rhs)
	{
		return !(operator==(lhs, rhs));
	}

}
// namespace AqualinkAutomate::Interfaces
