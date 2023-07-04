#include "formatters/orp_formatter.h"
#include "formatters/units_electric_potential_formatter.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Kernel::ORP& obj)
	{
		os << std::format("{}", obj);
		return os;
	}

}
// namespace std
