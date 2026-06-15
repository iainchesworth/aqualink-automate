#include <format>

#include <boost/program_options/value_semantic.hpp>

#include "logging/logging.h"
#include "options/options_matter_options.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::Matter
{

	boost::program_options::options_description OptionsProcessor::Options() const
	{
		boost::program_options::options_description options(SettingsType::AreaName(), Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", MatterOptionsCollection.size(), SettingsType::AreaName()));
		for (auto& option : MatterOptionsCollection)
		{
			options.add((*option)());
		}

		return options;
	}

	void OptionsProcessor::Validate(const boost::program_options::variables_map& vm) const
	{
		// No cross-option dependencies: every Matter option stands alone and the
		// sidecar tolerates 0 (generate) for passcode/discriminator.
		(void)vm;
	}

	std::expected<OptionsProcessor::SettingsType, ErrorCodes::Options_ErrorCodes> OptionsProcessor::Process(boost::program_options::variables_map& vm) const
	{
		SettingsType settings;

		if (OPTION_ENABLE->IsPresent(vm))
		{
			settings.enabled = OPTION_ENABLE->As<bool>(vm);
		}

		if (OPTION_STORAGE_PATH->IsPresent(vm))
		{
			settings.storage_path = OPTION_STORAGE_PATH->As<std::string>(vm);
		}

		if (OPTION_STATUS_PORT->IsPresent(vm))
		{
			settings.status_port = OPTION_STATUS_PORT->As<uint16_t>(vm);
		}

		if (OPTION_PASSCODE->IsPresent(vm))
		{
			settings.passcode = OPTION_PASSCODE->As<uint32_t>(vm);
		}

		if (OPTION_DISCRIMINATOR->IsPresent(vm))
		{
			settings.discriminator = OPTION_DISCRIMINATOR->As<uint16_t>(vm);
		}

		LogInfo(Channel::Options, std::format("Matter settings: enabled={}, storage_path={}, status_port={}",
			settings.enabled, settings.storage_path, settings.status_port));

		return settings;
	}

}
// namespace AqualinkAutomate::Options::Matter
