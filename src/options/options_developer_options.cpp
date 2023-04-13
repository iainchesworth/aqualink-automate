#include <format>
#include <string>

#include <boost/program_options/value_semantic.hpp>

#include "logging/logging.h"
#include "logging/logging_severity_filter.h"
#include "logging/formatters/logging_formatters.h"
#include "options/options_developer_options.h"
#include "options/options_option_type.h"
#include "options/validators/severity_level_validator.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::Developer
{
	AppOptionPtr OPTION_DEVMODE{ make_appoption("dev-mode", "Enable developer mode", boost::program_options::bool_switch()->default_value(false)) };
	AppOptionPtr OPTION_DEVREPLAYFILE{ make_appoption("replay-filename", "Developer replay file from which to source test data", boost::program_options::value<std::string>()) };
	AppOptionPtr OPTION_LOGLEVEL_MAIN{ make_appoption ("loglevel-main", "Set the logging level for Channel::Main", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken())};
	AppOptionPtr OPTION_LOGLEVEL_EXCEPTIONS{ make_appoption("loglevel-exceptions", "Set the logging level for Channel::Exceptions", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
	AppOptionPtr OPTION_LOGLEVEL_MESSAGES{ make_appoption("loglevel-messages", "Set the logging level for Channel::Messages", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
	AppOptionPtr OPTION_LOGLEVEL_OPTIONS{ make_appoption("loglevel-options", "Set the logging level for Channel::Options", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
	AppOptionPtr OPTION_LOGLEVEL_PLATFORM{ make_appoption("loglevel-platform", "Set the logging level for Channel::Platform", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
	AppOptionPtr OPTION_LOGLEVEL_SERIAL{ make_appoption("loglevel-serial", "Set the logging level for Channel::Serial", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
	AppOptionPtr OPTION_LOGLEVEL_SIGNALS{ make_appoption("loglevel-signals", "Set the logging level for Channel::Signals", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };

	std::vector DeveloperOptionsCollection
	{
		OPTION_DEVMODE,
		OPTION_DEVREPLAYFILE,
		OPTION_LOGLEVEL_MAIN,
		OPTION_LOGLEVEL_EXCEPTIONS,
		OPTION_LOGLEVEL_MESSAGES,
		OPTION_LOGLEVEL_OPTIONS,
		OPTION_LOGLEVEL_PLATFORM,
		OPTION_LOGLEVEL_SERIAL,
		OPTION_LOGLEVEL_SIGNALS
	};

	boost::program_options::options_description Options()
	{
		const std::string options_collection_name{ "Developer" };
		boost::program_options::options_description options(options_collection_name, Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", DeveloperOptionsCollection.size(), options_collection_name));
		for (auto& option : DeveloperOptionsCollection)
		{
			options.add((*option)());
		}

		return options;
	}

	Settings HandleOptions(boost::program_options::variables_map vm)
	{
		Settings settings;
		
		if (OPTION_DEVMODE->IsPresent(vm)) { settings.dev_mode_enabled = OPTION_DEVMODE->As<bool>(vm); }
		if (OPTION_DEVREPLAYFILE->IsPresent(vm)) { settings.replay_file = OPTION_DEVREPLAYFILE->As<std::string>(vm); }

		if (OPTION_LOGLEVEL_MAIN->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Main, OPTION_LOGLEVEL_MAIN->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_EXCEPTIONS->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Exceptions, OPTION_LOGLEVEL_EXCEPTIONS->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_MESSAGES->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Messages, OPTION_LOGLEVEL_MESSAGES->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_OPTIONS->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Options, OPTION_LOGLEVEL_OPTIONS->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_PLATFORM->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Platform, OPTION_LOGLEVEL_PLATFORM->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_SERIAL->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Serial, OPTION_LOGLEVEL_SERIAL->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_SIGNALS->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Signals, OPTION_LOGLEVEL_SIGNALS->As<Severity>(vm)); }

		return settings;
	}

}
// namespace AqualinkAutomate::Options::Developer
