#include "formatters/units_dimensionless_formatter.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Units::ppm_quantity& obj)
	{
		os << std::format("{}", obj);
		return os;
	}

}
// namespace std
