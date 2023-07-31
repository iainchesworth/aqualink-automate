#include <algorithm>
#include <format>
#include <string>

#include <boost/iterator/zip_iterator.hpp>
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
	AppOptionPtr OPTION_EMULATEDDEVICETYPE{ make_appoption("device-type", "Controller emulation type", boost::program_options::value<std::vector<Devices::JandyEmulatedDeviceTypes>>()->multitoken()) };
	AppOptionPtr OPTION_EMULATEDDEVICEID{ make_appoption("device-id", "Controller serial id", boost::program_options::value<std::vector<Devices::JandyDeviceId>>()->multitoken()) };

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
			auto device_ids = OPTION_EMULATEDDEVICEID->As< std::vector<Devices::JandyDeviceId>>(vm);

			if (device_types.size() != device_ids.size())
			{
				// Error
			}
			else
			{
				// This would be SOOOO much easier in C++23...
				// settings.emulated_devices = std::ranges::views::zip(device_types, device_ids);

				auto zip_begin = boost::make_zip_iterator(boost::make_tuple(device_types.begin(), device_ids.begin()));
				auto zip_end = boost::make_zip_iterator(boost::make_tuple(device_types.end(), device_ids.end()));

				std::transform(
					zip_begin, 
					zip_end, 
					std::back_inserter(settings.emulated_devices),
					[](const boost::tuple<Devices::JandyEmulatedDeviceTypes, Devices::JandyDeviceId>& t)
					{
						return JandyEmulatedDevice(t.get<0>(), t.get<1>());
					}
				);
			}
		}

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
