#include <format>

#include "logging/logging.h"
#include "options/options_pentair.h"
#include "options/helpers/conflicting_options_helper.h"
#include "options/helpers/option_dependency_helper.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Pentair::Options
{

	boost::program_options::options_description OptionsProcessor::Options() const
	{
		boost::program_options::options_description options(SettingsType::AreaName(), Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", PentairOptionsCollection.size(), SettingsType::AreaName()));
		for (auto& option : PentairOptionsCollection)
		{
			options.add((*option)());
		}

		return std::move(options);
	}

	void OptionsProcessor::Validate(const boost::program_options::variables_map& vm) const
	{
	}

	std::expected<OptionsProcessor::SettingsType, ErrorCodes::Options_ErrorCodes> OptionsProcessor::Process(boost::program_options::variables_map& vm) const
	{
		SettingsType settings;

		return std::move(settings);
	}

}
// namespace AqualinkAutomate::Pentair::Options
