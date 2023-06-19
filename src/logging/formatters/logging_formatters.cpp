#include <format>

#include <magic_enum.hpp>

#include "logging/logging.h"
#include "logging/formatters/logging_formatters.h"

namespace AqualinkAutomate::Logging
{

	std::istream& operator>>(std::istream& istream, AqualinkAutomate::Logging::Severity& v)
	{
		std::string enum_name;
		istream >> enum_name;

		if (auto enum_value = magic_enum::enum_cast<AqualinkAutomate::Logging::Severity>(enum_name); enum_value.has_value())
		{
			v = enum_value.value();
		}
		else
		{
			LogDebug(Channel::Main, std::format("Invalid conversion of severity level -> provided string was: {}", enum_name));
		}

		return istream;
	}

}
// namespace AqualinkAutomate::Logging
