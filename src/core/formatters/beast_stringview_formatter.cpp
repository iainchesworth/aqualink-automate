#include "formatters/beast_stringview_formatter.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	auto operator<<(std::ostream& os, const boost::beast::string_view& obj) -> std::ostream&
	{
		os << std::format("{}", obj);
		return os;
	}

}
// namespace std
