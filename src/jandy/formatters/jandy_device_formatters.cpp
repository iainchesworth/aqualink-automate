#include "jandy/formatters/jandy_device_formatters.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Devices::JandyDeviceId& obj)
	{
		os << obj();
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Devices::JandyDeviceType& obj)
	{
		os << obj.Id();
		return os;
	}

}
// namespace std
