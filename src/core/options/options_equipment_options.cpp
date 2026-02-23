#include <format>
#include <string>

#include "logging/logging.h"
#include "options/options_equipment_options.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::Equipment
{

	boost::program_options::options_description OptionsProcessor::Options() const
	{
		boost::program_options::options_description options(SettingsType::AreaName(), Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", EquipmentOptionsCollection.size(), SettingsType::AreaName()));
		for (auto& option : EquipmentOptionsCollection)
		{
			options.add((*option)());
		}

		return options;
	}

	void OptionsProcessor::Validate(const boost::program_options::variables_map& vm) const
	{
		// Validation is performed during Process(); nothing to check here
		// since boost::program_options already validates presence.
	}

	std::expected<OptionsProcessor::SettingsType, ErrorCodes::Options_ErrorCodes> OptionsProcessor::Process(boost::program_options::variables_map& vm) const
	{
		SettingsType settings;

		if (OPTION_POOL_CONFIGURATION->IsPresent(vm))
		{
			auto value = OPTION_POOL_CONFIGURATION->As<std::string>(vm);

			if (value == "pool-only" || value == "spa-only")
			{
				settings.pool_configuration = Kernel::PoolConfigurations::SingleBody;
				settings.pool_configuration_is_user_specified = true;
			}
			else if (value == "combo")
			{
				settings.pool_configuration = Kernel::PoolConfigurations::DualBody_SharedEquipment;
				settings.pool_configuration_is_user_specified = true;
			}
			else if (value == "dual")
			{
				settings.pool_configuration = Kernel::PoolConfigurations::DualBody_DualEquipment;
				settings.pool_configuration_is_user_specified = true;
			}
			else
			{
				// "auto" — leave as Unknown, not user-specified
				settings.pool_configuration = Kernel::PoolConfigurations::Unknown;
				settings.pool_configuration_is_user_specified = false;
			}
		}

		return settings;
	}

}
// namespace AqualinkAutomate::Options::Equipment
