#include "formatters/asio_endpoint_formatter.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const boost::asio::ip::tcp::endpoint& obj)
	{
		os << std::format("{}", obj);
		return os;
	}

}
// namespace std
