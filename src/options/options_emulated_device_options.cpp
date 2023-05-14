#include <format>
#include <string>

#include <boost/program_options/value_semantic.hpp>
#include <magic_enum.hpp>

#include "logging/logging.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/devices/jandy_emulated_device_types.h"
#include "options/options_emulated_device_options.h"
#include "options/options_option_type.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::Emulated
{
	AppOptionPtr OPTION_DISABLEEMULATION{ make_appoption("disable-emulation", "Disable Aqualink controller emulation", boost::program_options::bool_switch()->default_value(false)) };
	AppOptionPtr OPTION_EMULATEDDEVICETYPE{ make_appoption("device-type", "Controller emulation type", boost::program_options::value<std::string>()) };
	AppOptionPtr OPTION_EMULATEDDEVICEID{ make_appoption("device-id", "Controller serial id", boost::program_options::value<uint8_t>()->default_value(0x42)) };

	std::vector EmulatedDeviceOptionsCollection
	{
		OPTION_DISABLEEMULATION,
		OPTION_EMULATEDDEVICETYPE,
		OPTION_EMULATEDDEVICEID
	};

	boost::program_options::options_description Options()
	{
		const std::string options_collection_name{ "Emulation" };
		boost::program_options::options_description options(options_collection_name, Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", EmulatedDeviceOptionsCollection.size(), options_collection_name));
		for (auto& option : EmulatedDeviceOptionsCollection)
		{
			options.add((*option)());
		}

		return options;
	}

	Settings HandleOptions(boost::program_options::variables_map vm)
	{
		Settings settings;

		if (OPTION_DISABLEEMULATION->IsPresent(vm)) { settings.disable_emulation = OPTION_DISABLEEMULATION->As<bool>(vm); }
		if (OPTION_EMULATEDDEVICETYPE->IsPresent(vm)) { settings.device_type = magic_enum::enum_cast<Devices::JandyEmulatedDeviceTypes>(OPTION_EMULATEDDEVICETYPE->As<std::string>(vm)).value_or(Devices::JandyEmulatedDeviceTypes::Unknown); }
		if (OPTION_EMULATEDDEVICEID->IsPresent(vm)) { settings.device_id = Devices::JandyDeviceType(OPTION_EMULATEDDEVICEID->As<uint8_t>(vm)); }

		return settings;
	}

}
// namespace AqualinkAutomate::Options::Emulated
