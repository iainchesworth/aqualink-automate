#include <algorithm>
#include <format>
#include <ranges>

#include <magic_enum/magic_enum.hpp>

#include "devices/jandy_device_id.h"
#include "formatters/jandy_device_formatters.h"
#include "logging/logging.h"
#include "options/options_jandy.h"
#include "options/helpers/conflicting_options_helper.h"
#include "options/helpers/option_dependency_helper.h"
#include "options/validators/jandy_device_id_validator.h"
#include "options/validators/jandy_emulated_device_type_validator.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Jandy::Options
{

	boost::program_options::options_description OptionsProcessor::Options() const
	{
		boost::program_options::options_description options(SettingsType::AreaName(), Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", JandyOptionsCollection.size(), SettingsType::AreaName()));
		for (auto& option : JandyOptionsCollection)
		{
			options.add((*option)());
		}

		return std::move(options);
	}

	void OptionsProcessor::Validate(const boost::program_options::variables_map& vm) const
	{
		Helper_ValidateOptionDependencies(vm, OPTION_EMULATEDDEVICETYPE, OPTION_EMULATEDDEVICEID);
		Helper_ValidateOptionDependencies(vm, OPTION_EMULATEDDEVICEID, OPTION_EMULATEDDEVICETYPE);

		Helper_CheckForConflictingOptions(vm, OPTION_DISABLEEMULATION, OPTION_EMULATEDDEVICETYPE);
		Helper_CheckForConflictingOptions(vm, OPTION_DISABLEEMULATION, OPTION_EMULATEDDEVICEID);
	}

	std::expected<OptionsProcessor::SettingsType, ErrorCodes::Options_ErrorCodes> OptionsProcessor::Process(boost::program_options::variables_map& vm) const
	{
		SettingsType settings;

		if (OPTION_DISABLEEMULATION->IsPresent(vm))
		{
			settings.disable_emulation = OPTION_DISABLEEMULATION->As<bool>(vm);
		}

		if (!OPTION_EMULATEDDEVICETYPE->IsPresent(vm))
		{
			// No emulated devices....ignore.
		}
		else if (!OPTION_EMULATEDDEVICEID->IsPresent(vm))
		{
			// No emulated device ids....raise an error.
		}
		else
		{
			auto device_types = OPTION_EMULATEDDEVICETYPE->As<std::vector<Devices::JandyEmulatedDeviceTypes>>(vm);
			auto device_ids = OPTION_EMULATEDDEVICEID->As<std::vector<Devices::JandyDeviceId>>(vm);

			if (device_types.size() != device_ids.size())
			{
				// Error
			}
			else
			{
				std::ranges::transform(
					std::views::zip(device_types, device_ids),
					std::back_inserter(settings.emulated_devices),
					[](auto pair)
					{
						auto& [type, id] = pair;
						LogDebug(Channel::Options, std::format("Emplacing emulated device: {} (id: {})", magic_enum::enum_name(type), id));
						return JandyEmulatedDevice(type, id);
					}
				);
			}
		}

		return std::move(settings);
	}

}
// namespace AqualinkAutomate::Jandy::Options
