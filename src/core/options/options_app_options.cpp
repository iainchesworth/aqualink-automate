#include <format>
#include <iostream>
#include <string>

#include "exceptions/exception_optionshelporversion.h"
#include "logging/logging.h"
#include "logging/logging_severity_filter.h"
#include "options/options_app_options.h"
#include "options/options_option_type.h"
#include "options/helpers/build_options_description.h"
#include "options/helpers/conflicting_options_helper.h"
#include "version/version.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::App
{

	boost::program_options::options_description OptionsProcessor::Options() const
	{
		return BuildOptionsDescription(SettingsType::AreaName(), AppOptionsCollection);
	}

	void OptionsProcessor::Validate(const boost::program_options::variables_map& vm) const
	{
		Helper_CheckForConflictingOptions(vm, OPTION_DEBUG, OPTION_TRACE);
	}

	std::expected<OptionsProcessor::SettingsType, ErrorCodes::Options_ErrorCodes> OptionsProcessor::Process(boost::program_options::variables_map& vm) const
	{
		SettingsType settings;

		if (OPTION_CONFIG->IsPresent(vm))
		{
			settings.config_file = OPTION_CONFIG->As<std::string>(vm);
		}

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

	void HandleHelp(boost::program_options::variables_map& vm, boost::program_options::options_description& options)
	{
		// Query the variables_map directly by the declared option long name
		// rather than reconstructing a throwaway AppOption from string literals.
		if (0 < vm.count("help"))
		{
			// Display the help information to the user.
			std::cout << options << '\n';

			// Terminate the application...
			throw Exceptions::OptionsHelpOrVersion();
		}
		else
		{
			LogTrace(Channel::Options, "Help option not provided; doing nothing...");
		}
	}

	void HandleVersion(boost::program_options::variables_map& vm)
	{
		if (0 < vm.count("version-detail"))
		{
			// Display the version information to the user.
			std::cout << Version::VersionDetails() << '\n' << Version::GitCommitDetails() << '\n';

			// Terminate the application...
			throw Exceptions::OptionsHelpOrVersion();
		}
		else if (0 < vm.count("version"))
		{
			const auto version_info = std::format
			(
				"{} v{}\n{}",
				Version::VersionInfo::ProjectName(),
				Version::VersionInfo::ProjectVersionFull(),
				Version::VersionInfo::ProjectDescription()
			);

			// Display the version information to the user.
			std::cout << version_info << '\n';

			// Terminate the application...
			throw Exceptions::OptionsHelpOrVersion();
		}
		else
		{
			LogTrace(Channel::Options, "Version option not provided; doing nothing...");
		}
	}

	void HandleHelpAndVersion(boost::program_options::variables_map& vm, boost::program_options::options_description& options)
	{
		HandleHelp(vm, options);
		HandleVersion(vm);
	}

}
// namespace AqualinkAutomate::Options::App
