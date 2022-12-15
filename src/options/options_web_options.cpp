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
	AppOptionPtr OPTION_INTERFACE{ make_appoption("interface", "Specific IP address to which to bind", boost::program_options::value<std::string>()->default_value("127.0.0.1")) };
	AppOptionPtr OPTION_PORT{ make_appoption("port", "Specific port number on which to listen", boost::program_options::value<uint16_t>()->default_value(80)) };

	std::vector WebOptionsCollection
	{
		OPTION_INTERFACE,
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

	void HandleOptions(boost::program_options::variables_map vm)
	{

	}

}
// namespace AqualinkAutomate::Options::Web
