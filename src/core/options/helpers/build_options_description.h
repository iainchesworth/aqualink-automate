#pragma once

#include <format>
#include <string>
#include <vector>

#include <boost/program_options/options_description.hpp>

#include "logging/logging.h"
#include "options/options_option_type.h"
#include "utility/get_terminal_column_width.h"

namespace AqualinkAutomate::Options
{

	// Shared body for every OptionsProcessor::Options() override: builds an
	// options_description titled with the area name (at the current terminal
	// width) and adds each AppOption in the supplied collection. Centralises the
	// previously copy-pasted loop/log so each processor's Options() is one line.
	//
	// Header-only (inline) so no new translation unit / CMake entry is needed.
	[[nodiscard]] inline boost::program_options::options_description BuildOptionsDescription(const std::string& area_name, const std::vector<AppOptionPtr>& options_collection)
	{
		using namespace AqualinkAutomate::Logging;

		boost::program_options::options_description options(area_name, Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", options_collection.size(), area_name));
		for (const auto& option : options_collection)
		{
			options.add((*option)());
		}

		return options;
	}

}
// namespace AqualinkAutomate::Options
