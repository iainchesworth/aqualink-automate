#include <format>
#include <iostream>
#include <string>

#include "exceptions/exception_optionshelporversion.h"
#include "logging/logging.h"
#include "logging/logging_severity_filter.h"
#include "options/options_app_options.h"
#include "options/options_option_type.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::App
{
	AppOptionPtr OPTION_DEBUG{ make_appoption("debug", "d", "Enable debug logging") };
	AppOptionPtr OPTION_HELP{ make_appoption("help", "h", "Displays the help information") };
	AppOptionPtr OPTION_TRACE{ make_appoption("trace", "Enable trace logging") };
	AppOptionPtr OPTION_VERSION{ make_appoption("version", "v", "Displays the version information") };

	const std::vector AppOptionsCollection
	{
		OPTION_DEBUG,
		OPTION_HELP,
		OPTION_TRACE,
		OPTION_VERSION
	};
	
	boost::program_options::options_description Options()
	{
		const std::string options_collection_name{ "App" };
		boost::program_options::options_description options(options_collection_name, Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", AppOptionsCollection.size(), options_collection_name));
		for (auto& option : AppOptionsCollection)
		{
			options.add((*option)());
		}

		return options;
	}

	void HandleHelp(boost::program_options::variables_map vm, boost::program_options::options_description& options)
	{
		if (OPTION_HELP->IsPresent(vm))
		{
			// Display the help information to the user.
			std::cout << options << std::endl;

			// Terminate the application...
			throw Exceptions::OptionsHelpOrVersion();
		}
		else
		{
			LogTrace(Channel::Options, "Help option not provided; doing nothing...");
		}
	}

	Settings HandleOptions(boost::program_options::variables_map vm)
	{
		Settings settings;

		if (OPTION_DEBUG->IsPresent(vm))
		{
			LogTrace(Channel::Options, "Setting global logging severity level filter to Debug");
			Logging::SeverityFiltering::SetGlobalFilterLevel(Severity::Debug);
		}
		else if (OPTION_TRACE->IsPresent(vm))
		{
			LogTrace(Channel::Options, "Setting global logging severity level filter to Trace");
			Logging::SeverityFiltering::SetGlobalFilterLevel(Severity::Trace);
		}
		else
		{
			// Do nothing...
		}

		return settings;
	}

	void HandleVersion(boost::program_options::variables_map vm)
	{
		if (OPTION_VERSION->IsPresent(vm))
		{
			// Display the version information to the user.

			// Terminate the application...
			throw Exceptions::OptionsHelpOrVersion();
		}
		else
		{
			LogTrace(Channel::Options, "Version option not provided; doing nothing...");
		}
	}

}
// namespace AqualinkAutomate::Options::App
