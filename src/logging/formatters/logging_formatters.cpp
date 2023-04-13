#include <format>

#include <boost/describe.hpp>
#include <boost/describe/enum_from_string.hpp>

#include "logging/logging.h"
#include "logging/formatters/logging_formatters.h"

namespace AqualinkAutomate::Logging
{

	std::istream& operator>>(std::istream& istream, AqualinkAutomate::Logging::Severity& v)
	{
		std::string enum_name;
		istream >> enum_name;

		if (!boost::describe::enum_from_string(enum_name.c_str(), v))
		{
			LogDebug(Channel::Main, std::format("Invalid conversion of severity level -> provided string was: {}", enum_name));
		}

		return istream;
	}

}
// namespace AqualinkAutomate::Logging
