#include <format>
#include <string>

#include <boost/program_options/value_semantic.hpp>

#include "logging/logging.h"
#include "options/options_developer_options.h"
#include "options/options_option_type.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::Developer
{
	// AppOptionPtr OPTION_DEVMODE{ make_appoption("dev-mode", "Enable developer mode", boost::program_options::bool_switch())};
	AppOptionPtr OPTION_DEVREPLAYFILE{ make_appoption("replay-filename", "Developer replay file from which to source test data", boost::program_options::value<std::string>()) };
	// AppOptionPtr OPTION_DEVREPLAYSERIAL{ make_appoption("replay-serialport", "Developer replay serial port from which to source test data", boost::program_options::value<std::string>()->default_value("COM5")) };
	// AppOptionPtr OPTION_DEVREPLAYBAUDRATE{ make_appoption("replay", "Desired developer replay serial port baud rate setting", boost::program_options::value<uint32_t>()->default_value(9600)) };

	std::vector DeveloperOptionsCollection
	{
		/*OPTION_DEVMODE,
		OPTION_DEVSERIALPORT,
		OPTION_DEVBAUDRATE*/
		OPTION_DEVREPLAYFILE
	};

	boost::program_options::options_description Options()
	{
		const std::string options_collection_name{ "Developer" };
		boost::program_options::options_description options(options_collection_name, Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", DeveloperOptionsCollection.size(), options_collection_name));
		for (auto& option : DeveloperOptionsCollection)
		{
			options.add((*option)());
		}

		return options;
	}

	Settings HandleOptions(boost::program_options::variables_map vm)
	{
		Settings settings;
		
		// if (OPTION_DEVMODE->IsPresent(vm)) { settings.dev_mode_enabled = OPTION_DEVMODE->As<bool>(vm); }
		// if (OPTION_DEVSERIALPORT->IsPresent(vm)) { settings.dev_serial_port = OPTION_DEVSERIALPORT->As<std::string>(vm); }

		if (OPTION_DEVREPLAYFILE->IsPresent(vm)) { settings.replay_file = OPTION_DEVREPLAYFILE->As<std::string>(vm); }

		return settings;
	}

}
// namespace AqualinkAutomate::Options::Developer
