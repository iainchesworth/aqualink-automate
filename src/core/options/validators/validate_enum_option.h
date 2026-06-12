#pragma once

#include <format>
#include <string>
#include <vector>

#include <boost/any.hpp>
#include <boost/program_options/errors.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <magic_enum/magic_enum.hpp>

#include "logging/logging.h"
#include "utility/case_insensitive_comparision.h"

namespace AqualinkAutomate::Options::Validators
{

	//=========================================================================
	//
	// Shared helper backing the per-type boost::program_options `validate()`
	// overloads (Severity, ProfilerTypes, ...).
	//
	// It centralises the single-occurrence check, the (empty-safe,
	// case-insensitive) enum_cast and the standard "invalid option value"
	// failure path so each validator is a one-line forwarder.
	//
	// Empty / whitespace-only / unknown values fail cleanly via
	// validation_error -- there is NO indexing of the input string, so an
	// empty option value (e.g. `--loglevel-main ""` or a bare `loglevel-main =`
	// config line) can never read out of bounds. The case-insensitive compare
	// is fed unsigned char by Utility::CaseInsensitiveComparison, avoiding the
	// UB of passing a (possibly negative) char to std::tolower.
	//
	//=========================================================================

	template<typename ENUM>
	void ValidateEnumOption(boost::any& v, const std::vector<std::string>& values, AqualinkAutomate::Logging::Channel log_channel, std::string_view enum_label)
	{
		using namespace AqualinkAutomate::Logging;

		boost::program_options::validators::check_first_occurrence(v);
		const std::string& option_string = boost::program_options::validators::get_single_string(values);

		// Case-insensitive match so e.g. `trace`/`TRACE`/`Trace` all resolve to
		// the canonical PascalCase enumerator. magic_enum::enum_cast returns an
		// empty optional for an empty or unmatched string, so there is no need
		// to (and we must not) index option_string[0].
		const auto comparison = [](char lhs, char rhs)
			{
				return Utility::CaseInsensitiveComparison(static_cast<unsigned char>(lhs), static_cast<unsigned char>(rhs));
			};

		if (const auto enum_value = magic_enum::enum_cast<ENUM>(option_string, comparison); enum_value.has_value())
		{
			v = boost::any(enum_value.value());
		}
		else
		{
			LogDebug(log_channel, std::format("Invalid conversion of {} -> provided string was: '{}'", enum_label, option_string));
			throw boost::program_options::validation_error(boost::program_options::validation_error::invalid_option_value);
		}
	}

}
// namespace AqualinkAutomate::Options::Validators
