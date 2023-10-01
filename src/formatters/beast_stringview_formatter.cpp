#include "formatters/beast_stringview_formatter.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const boost::beast::string_view& obj)
	{
		os << std::format("{}", obj);
		return os;
	}

}
// namespace std
