#include <algorithm>
#include <format>
#include <ranges>

#include <boost/program_options.hpp>
#include <magic_enum.hpp>

#include "logging/logging.h"
#include "options/validators/profiler_type_validator.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::Validators
{

	// NOTHING HERE

}
// namespace AqualinkAutomate::Options::Validators

namespace AqualinkAutomate::Types
{

	void validate(boost::any& v, std::vector<std::string> const& values, ProfilerTypes* target_type, int)
	{
		std::string profiler_string;

		boost::program_options::validators::check_first_occurrence(v);
		profiler_string = boost::program_options::validators::get_single_string(values);

		// To accomodate the enum_from_string needing to have the string match the enum exactly, including case,
		// the provided level needs to be case-insensitively compared e.g. vtune -> VTune or tracy -> Tracy

		auto case_insensitive_comp = [](char lhs, char rhs) { return std::tolower(lhs) == std::tolower(rhs); };

		if (auto enum_value = magic_enum::enum_cast<ProfilerTypes>(profiler_string, case_insensitive_comp); enum_value.has_value())
		{
			v = boost::any(enum_value.value());
		}
		else
		{
			LogDebug(Channel::Main, std::format("Invalid conversion of profiler -> provided string was: {}", profiler_string));
			throw boost::program_options::validation_error(boost::program_options::validation_error::invalid_option_value);
		}
	}

}
// namespace AqualinkAutomate::Types
