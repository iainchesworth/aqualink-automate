#include <format>

#include <boost/program_options/value_semantic.hpp>

#include "logging/logging.h"
#include "options/options_scheduling_options.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::Scheduling
{

	boost::program_options::options_description OptionsProcessor::Options() const
	{
		boost::program_options::options_description options(SettingsType::AreaName(), Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", SchedulingOptionsCollection.size(), SettingsType::AreaName()));
		for (auto& option : SchedulingOptionsCollection)
		{
			options.add((*option)());
		}

		return options;
	}

	void OptionsProcessor::Validate(const boost::program_options::variables_map& /*vm*/) const
	{
		// v1: --schedules-file only; nothing to cross-validate.
	}

	std::expected<OptionsProcessor::SettingsType, ErrorCodes::Options_ErrorCodes> OptionsProcessor::Process(boost::program_options::variables_map& vm) const
	{
		SettingsType settings;

		if (OPTION_SCHEDULES_FILE->IsPresent(vm))
		{
			settings.schedules_file = OPTION_SCHEDULES_FILE->As<std::string>(vm);
		}

		return settings;
	}

}
// namespace AqualinkAutomate::Options::Scheduling
