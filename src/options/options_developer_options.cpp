#include <format>
#include <string>

#include <boost/program_options/value_semantic.hpp>

#include "logging/logging.h"
#include "logging/logging_severity_filter.h"
#include "logging/formatters/logging_formatters.h"
#include "options/options_developer_options.h"
#include "options/options_option_type.h"
#include "options/helpers/conflicting_options_helper.h"
#include "options/helpers/option_dependency_helper.h"
#include "options/validators/profiler_type_validator.h"
#include "options/validators/severity_level_validator.h"
#include "profiling/factories/profiler_factory.h"
#include "profiling/types/profiling_types.h"
#include "profiling/formatters/profiling_formatters.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::Developer
{
	AppOptionPtr OPTION_DEVMODE{ make_appoption("dev-mode", "Enable developer mode", boost::program_options::bool_switch()->default_value(false)) };
	AppOptionPtr OPTION_DEVREPLAYFILE{ make_appoption("replay-filename", "Developer replay file from which to source test data", boost::program_options::value<std::string>()) };
	AppOptionPtr OPTION_LOGLEVEL_MAIN{ make_appoption ("loglevel-main", "Set the logging level for Channel::Main", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken())};
	AppOptionPtr OPTION_LOGLEVEL_CERTIFICATES{ make_appoption("loglevel-certificates", "Set the logging level for Channel::Certificates", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
	AppOptionPtr OPTION_LOGLEVEL_DEVICES{ make_appoption("loglevel-devices", "Set the logging level for Channel::Devices", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) }; 
	AppOptionPtr OPTION_LOGLEVEL_EQUIPMENT{ make_appoption("loglevel-equipment", "Set the logging level for Channel::Equipment", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) }; 
	AppOptionPtr OPTION_LOGLEVEL_EXCEPTIONS{ make_appoption("loglevel-exceptions", "Set the logging level for Channel::Exceptions", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
	AppOptionPtr OPTION_LOGLEVEL_MESSAGES{ make_appoption("loglevel-messages", "Set the logging level for Channel::Messages", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
	AppOptionPtr OPTION_LOGLEVEL_OPTIONS{ make_appoption("loglevel-options", "Set the logging level for Channel::Options", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
	AppOptionPtr OPTION_LOGLEVEL_PLATFORM{ make_appoption("loglevel-platform", "Set the logging level for Channel::Platform", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
	AppOptionPtr OPTION_LOGLEVEL_PROFILING{ make_appoption("loglevel-profiling", "Set the logging level for Channel::Profiling", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
	AppOptionPtr OPTION_LOGLEVEL_PROTCOL{ make_appoption("loglevel-protocol", "Set the logging level for Channel::Protocol", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) }; 
	AppOptionPtr OPTION_LOGLEVEL_SERIAL{ make_appoption("loglevel-serial", "Set the logging level for Channel::Serial", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
	AppOptionPtr OPTION_LOGLEVEL_SIGNALS{ make_appoption("loglevel-signals", "Set the logging level for Channel::Signals", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
	AppOptionPtr OPTION_LOGLEVEL_WEB{ make_appoption("loglevel-web", "Set the logging level for Channel::Web", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
	AppOptionPtr OPTION_PROFILER{ make_appoption("profiler", "Enabling profiling using specified profiling tool", boost::program_options::value<AqualinkAutomate::Types::ProfilerTypes>()->multitoken()) };

	std::vector DeveloperOptionsCollection
	{
		OPTION_DEVMODE,
		OPTION_DEVREPLAYFILE,
		OPTION_LOGLEVEL_MAIN,
		OPTION_LOGLEVEL_CERTIFICATES,
		OPTION_LOGLEVEL_DEVICES,
		OPTION_LOGLEVEL_EQUIPMENT,
		OPTION_LOGLEVEL_EXCEPTIONS,
		OPTION_LOGLEVEL_MESSAGES,
		OPTION_LOGLEVEL_OPTIONS,
		OPTION_LOGLEVEL_PLATFORM,
		OPTION_LOGLEVEL_PROFILING,
		OPTION_LOGLEVEL_PROTCOL,
		OPTION_LOGLEVEL_SERIAL,
		OPTION_LOGLEVEL_SIGNALS,
		OPTION_LOGLEVEL_WEB,
		OPTION_PROFILER
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

	Settings HandleOptions(boost::program_options::variables_map& vm)
	{
		Settings settings;
		
		if (OPTION_DEVMODE->IsPresent(vm)) { settings.dev_mode_enabled = OPTION_DEVMODE->As<bool>(vm); }
		if (OPTION_DEVREPLAYFILE->IsPresent(vm)) { settings.replay_file = OPTION_DEVREPLAYFILE->As<std::string>(vm); }

		if (OPTION_LOGLEVEL_MAIN->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Main, OPTION_LOGLEVEL_MAIN->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_CERTIFICATES->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Certificates, OPTION_LOGLEVEL_CERTIFICATES->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_DEVICES->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Devices, OPTION_LOGLEVEL_DEVICES->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_EQUIPMENT->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Equipment, OPTION_LOGLEVEL_EQUIPMENT->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_EXCEPTIONS->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Exceptions, OPTION_LOGLEVEL_EXCEPTIONS->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_MESSAGES->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Messages, OPTION_LOGLEVEL_MESSAGES->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_OPTIONS->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Options, OPTION_LOGLEVEL_OPTIONS->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_PLATFORM->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Platform, OPTION_LOGLEVEL_PLATFORM->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_PROFILING->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Protocol, OPTION_LOGLEVEL_PROFILING->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_PROTCOL->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Protocol, OPTION_LOGLEVEL_PROTCOL->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_SERIAL->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Serial, OPTION_LOGLEVEL_SERIAL->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_SIGNALS->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Signals, OPTION_LOGLEVEL_SIGNALS->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_WEB->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Web, OPTION_LOGLEVEL_WEB->As<Severity>(vm)); }

		if (OPTION_PROFILER->IsPresent(vm)) { Factory::ProfilerFactory::Instance().SetDesired(OPTION_PROFILER->As<Types::ProfilerTypes>(vm)); }

		return settings;
	}

	void ValidateOptions(boost::program_options::variables_map& vm)
	{
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_DEVMODE);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_MAIN);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_CERTIFICATES);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_DEVICES);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_EQUIPMENT);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_EXCEPTIONS);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_MESSAGES);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_OPTIONS);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_PLATFORM);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_PROFILING);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_PROTCOL);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_SERIAL);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_SIGNALS);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_WEB);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_PROFILER);
	}

}
// namespace AqualinkAutomate::Options::Developer
