#include "jandy/formatters/chemistry_formatter.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Utility::Chemistry& obj)
	{
		os << std::format("{}", obj);
		return os;
	}

}
// namespace std
