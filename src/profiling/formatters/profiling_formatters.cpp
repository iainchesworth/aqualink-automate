#include <format>

#include <magic_enum.hpp>

#include "logging/logging.h"
#include "profiling/formatters/profiling_formatters.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Profiling
{

	std::istream& operator>>(std::istream& istream, AqualinkAutomate::Profiling::ProfilerTypes& v)
	{
		std::string enum_name;
		istream >> enum_name;

		if (auto enum_value = magic_enum::enum_cast<AqualinkAutomate::Profiling::ProfilerTypes>(enum_name); enum_value.has_value())
		{
			v = enum_value.value();
		}
		else
		{
			LogDebug(Channel::Main, std::format("Invalid conversion of profiler type -> provided string was: {}", enum_name));
		}

		return istream;
	}

}
// namespace AqualinkAutomate::Profiling
