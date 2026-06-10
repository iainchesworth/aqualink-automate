#include <format>

#include <boost/program_options/value_semantic.hpp>

#include "logging/logging.h"
#include "options/options_preferences_options.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::Preferences
{

	boost::program_options::options_description OptionsProcessor::Options() const
	{
		boost::program_options::options_description options(SettingsType::AreaName(), Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", PreferencesOptionsCollection.size(), SettingsType::AreaName()));
		for (auto& option : PreferencesOptionsCollection)
		{
			options.add((*option)());
		}

		return options;
	}

	void OptionsProcessor::Validate(const boost::program_options::variables_map& /*vm*/) const
	{
		// Only --preferences-file; nothing to cross-validate.
	}

	std::expected<OptionsProcessor::SettingsType, ErrorCodes::Options_ErrorCodes> OptionsProcessor::Process(boost::program_options::variables_map& vm) const
	{
		SettingsType settings;

		if (OPTION_PREFERENCES_FILE->IsPresent(vm))
		{
			settings.preferences_file = OPTION_PREFERENCES_FILE->As<std::string>(vm);
		}

		return settings;
	}

}
// namespace AqualinkAutomate::Options::Preferences
