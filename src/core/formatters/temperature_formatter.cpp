#include "formatters/temperature_formatter.h"

#include "formatters/formatter_helpers.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Kernel::Temperature& obj)
	{
		return AqualinkAutomate::Formatters::WriteFormatted(os, obj);
	}

}
// namespace std
