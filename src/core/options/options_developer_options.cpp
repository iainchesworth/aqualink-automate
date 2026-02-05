#include <format>
#include <string>

#include <boost/program_options/value_semantic.hpp>

#include "logging/logging.h"
#include "logging/logging_severity_filter.h"
#include "logging/formatters/logging_formatters.h"
#include "options/options_developer_options.h"
#include "options/helpers/conflicting_options_helper.h"
#include "options/helpers/option_dependency_helper.h"
#include "options/validators/profiler_type_validator.h"
#include "options/validators/severity_level_validator.h"
#include "profiling/profiling.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::Developer
{

	boost::program_options::options_description OptionsProcessor::Options() const
	{
		boost::program_options::options_description options(SettingsType::AreaName(), Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", DeveloperOptionsCollection.size(), SettingsType::AreaName()));
		for (auto& option : DeveloperOptionsCollection)
		{
			options.add((*option)());
		}

		return options;
	}

	void OptionsProcessor::Validate(const boost::program_options::variables_map& vm) const
	{
		Helper_CheckForConflictingOptions(vm, OPTION_DEVREPLAYFILE, OPTION_DEVRECORDFILE);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_DEVMODE);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_MAIN);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_CERTIFICATES);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_COROUTINES);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_DEVICES);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_EQUIPMENT);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_EXCEPTIONS);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_MESSAGES);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_NAVIGATION);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_OPTIONS);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_PLATFORM);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_PROFILING);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_PROTCOL);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_SCRAPING);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_SERIAL);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_SIGNALS);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_LOGLEVEL_WEB);
		Helper_ValidateOptionDependencies(vm, OPTION_DEVREPLAYFILE, OPTION_PROFILER);
	}

	std::expected<OptionsProcessor::SettingsType, ErrorCodes::Options_ErrorCodes> OptionsProcessor::Process(boost::program_options::variables_map& vm) const
	{
		SettingsType settings;
		
		if (OPTION_DEVMODE->IsPresent(vm)) { settings.dev_mode_enabled = OPTION_DEVMODE->As<bool>(vm); }
		if (OPTION_DEVREPLAYFILE->IsPresent(vm)) { settings.replay_file = OPTION_DEVREPLAYFILE->As<std::string>(vm); }
		if (OPTION_DEVRECORDFILE->IsPresent(vm)) { settings.recording_file = OPTION_DEVRECORDFILE->As<std::string>(vm); }

		if (OPTION_LOGLEVEL_MAIN->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Main, OPTION_LOGLEVEL_MAIN->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_CERTIFICATES->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Certificates, OPTION_LOGLEVEL_CERTIFICATES->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_COROUTINES->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Coroutines, OPTION_LOGLEVEL_COROUTINES->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_DEVICES->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Devices, OPTION_LOGLEVEL_DEVICES->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_EQUIPMENT->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Equipment, OPTION_LOGLEVEL_EQUIPMENT->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_EXCEPTIONS->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Exceptions, OPTION_LOGLEVEL_EXCEPTIONS->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_MESSAGES->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Messages, OPTION_LOGLEVEL_MESSAGES->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_NAVIGATION->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Navigation, OPTION_LOGLEVEL_NAVIGATION->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_OPTIONS->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Options, OPTION_LOGLEVEL_OPTIONS->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_PLATFORM->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Platform, OPTION_LOGLEVEL_PLATFORM->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_PROFILING->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Protocol, OPTION_LOGLEVEL_PROFILING->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_PROTCOL->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Protocol, OPTION_LOGLEVEL_PROTCOL->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_SCRAPING->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Scraping, OPTION_LOGLEVEL_SCRAPING->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_SERIAL->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Serial, OPTION_LOGLEVEL_SERIAL->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_SIGNALS->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Signals, OPTION_LOGLEVEL_SIGNALS->As<Severity>(vm)); }
		if (OPTION_LOGLEVEL_WEB->IsPresent(vm)) { SeverityFiltering::SetChannelFilterLevel(Channel::Web, OPTION_LOGLEVEL_WEB->As<Severity>(vm)); }

		if (OPTION_PROFILER->IsPresent(vm)) { Factory::ProfilerFactory::Instance().SetDesired(OPTION_PROFILER->As<Types::ProfilerTypes>(vm)); }

		return std::move(settings);
	}

}
// namespace AqualinkAutomate::Options::Developer
