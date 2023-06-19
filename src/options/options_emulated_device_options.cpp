#include <format>
#include <string>

#include <boost/program_options/value_semantic.hpp>
#include <magic_enum.hpp>

#include "logging/logging.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/devices/jandy_emulated_device_types.h"
#include "jandy/formatters/jandy_device_formatters.h"
#include "options/options_emulated_device_options.h"
#include "options/options_option_type.h"
#include "options/helpers/conflicting_options_helper.h"
#include "options/helpers/option_dependency_helper.h"
#include "options/validators/jandy_device_id_validator.h"
#include "options/validators/jandy_emulated_device_type_validator.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::Emulated
{
	AppOptionPtr OPTION_DISABLEEMULATION{ make_appoption("disable-emulation", "Disable Aqualink controller emulation", boost::program_options::bool_switch()->default_value(false)) };
	AppOptionPtr OPTION_EMULATEDDEVICETYPE{ make_appoption("device-type", "Controller emulation type", boost::program_options::value<Devices::JandyEmulatedDeviceTypes>()) };
	AppOptionPtr OPTION_EMULATEDDEVICEID{ make_appoption("device-id", "Controller serial id", boost::program_options::value<Devices::JandyDeviceId>()) };

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

	Settings HandleOptions(boost::program_options::variables_map& vm)
	{
		Settings settings;

		if (OPTION_DISABLEEMULATION->IsPresent(vm)) { settings.disable_emulation = OPTION_DISABLEEMULATION->As<bool>(vm); }
		if (OPTION_EMULATEDDEVICETYPE->IsPresent(vm)) { settings.controller_type = OPTION_EMULATEDDEVICETYPE->As<Devices::JandyEmulatedDeviceTypes>(vm); }
		if (OPTION_EMULATEDDEVICEID->IsPresent(vm)) { settings.device_type = Devices::JandyDeviceType(OPTION_EMULATEDDEVICEID->As<Devices::JandyDeviceId>(vm)); }

		return settings;
	}

	void ValidateOptions(boost::program_options::variables_map& vm)
	{
		Helper_ValidateOptionDependencies(vm, OPTION_EMULATEDDEVICETYPE, OPTION_EMULATEDDEVICEID);
		Helper_ValidateOptionDependencies(vm, OPTION_EMULATEDDEVICEID, OPTION_EMULATEDDEVICETYPE);

		Helper_CheckForConflictingOptions(vm, OPTION_DISABLEEMULATION, OPTION_EMULATEDDEVICETYPE);
		Helper_CheckForConflictingOptions(vm, OPTION_DISABLEEMULATION, OPTION_EMULATEDDEVICEID);
	}

}
// namespace AqualinkAutomate::Options::Emulated
