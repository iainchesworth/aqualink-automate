#include <algorithm>
#include <format>
#include <ranges>

#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <magic_enum.hpp>

#include "logging/logging.h"
#include "options/validators/severity_level_validator.h"

namespace AqualinkAutomate::Options::Validators
{

	// NOTHING HERE

}
// namespace AqualinkAutomate::Options::Validators

namespace AqualinkAutomate::Logging
{

	void validate(boost::any& v, std::vector<std::string> const& values, Severity* target_type, int)
	{
		auto capitalize = [](std::string_view word) 
		{
			auto first = std::toupper(word[0]);
			auto rest = std::views::drop(word, 1) | std::views::transform([](char c) { return std::tolower(c); });
			return std::string(1, first) + std::string(rest.begin(), rest.end());
		};

		std::string sev_string;
		
		boost::program_options::validators::check_first_occurrence(v);
		sev_string = boost::program_options::validators::get_single_string(values);

		// To accomodate the enum_from_string needing to have the string match the enum exactly, including case,
		// the provided level needs to be capitalised e.g. trace -> Trace or DEBUG -> Debug

		if (auto enum_value = magic_enum::enum_cast<AqualinkAutomate::Logging::Severity>(capitalize(sev_string)); enum_value.has_value())
		{
			v = boost::any(enum_value.value());
		}
		else
		{
			LogDebug(Channel::Main, std::format("Invalid conversion of severity level -> provided string was: {}", sev_string));
			throw boost::program_options::validation_error(boost::program_options::validation_error::invalid_option_value);
		}
	}

}
// namespace AqualinkAutomate::Logging
