#include "formatters/chemistry_formatter.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Utility::ChemistryStringConverter& obj)
	{
		os << std::format("{}", obj);
		return os;
	}

}
// namespace std
