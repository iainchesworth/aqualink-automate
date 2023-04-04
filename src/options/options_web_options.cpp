#include <cstdint>
#include <format>
#include <string>

#include "logging/logging.h"
#include "options/options_option_type.h"
#include "options/options_web_options.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::Web
{
	AppOptionPtr OPTION_ADDRESS{ make_appoption("address", "Specific IP address to which to bind", boost::program_options::value<std::string>()->default_value("0.0.0.0")) };
	AppOptionPtr OPTION_PORT{ make_appoption("port", "Specific port number on which to listen", boost::program_options::value<uint16_t>()->default_value(80)) };

	std::vector WebOptionsCollection
	{
		OPTION_ADDRESS,
		OPTION_PORT
	};

	boost::program_options::options_description Options()
	{
		const std::string options_collection_name{ "Web" };
		boost::program_options::options_description options(options_collection_name, Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", WebOptionsCollection.size(), options_collection_name));
		for (auto& option : WebOptionsCollection)
		{
			options.add((*option)());
		}

		return options;
	}

	Settings HandleOptions(boost::program_options::variables_map vm)
	{
		Settings settings;

		if (OPTION_ADDRESS->IsPresent(vm)) { settings.address = boost::asio::ip::make_address(OPTION_ADDRESS->As<std::string>(vm)); }
		if (OPTION_PORT->IsPresent(vm)) { settings.port = OPTION_PORT->As<uint16_t>(vm); }

		return settings;
	}

}
// namespace AqualinkAutomate::Options::Web
