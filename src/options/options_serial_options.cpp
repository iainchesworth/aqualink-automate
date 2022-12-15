#include <format>
#include <string>

#include <boost/program_options/value_semantic.hpp>

#include "logging/logging.h"
#include "options/options_option_type.h"
#include "options/options_serial_options.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::Serial
{
	AppOptionPtr OPTION_SERIALPORT{ make_appoption("serial-port", "s", "Serial port to use for Aqualink connectivity", boost::program_options::value<std::string>()->default_value("COM1")) };
	AppOptionPtr OPTION_BAUDRATE{ make_appoption("baudrate", "Desired serial port baud rate setting", boost::program_options::value<uint32_t>()->default_value(9600)) };

	std::vector SerialOptionsCollection
	{
		OPTION_SERIALPORT,
		OPTION_BAUDRATE
	};

	boost::program_options::options_description Options()
	{
		const std::string options_collection_name{ "Serial" };
		boost::program_options::options_description options(options_collection_name, Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", SerialOptionsCollection.size(), options_collection_name));
		for (auto& option : SerialOptionsCollection)
		{
			options.add((*option)());
		}

		return options;
	}

	void HandleOptions(boost::program_options::variables_map vm)
	{

	}

}
// namespace AqualinkAutomate::Options::Serial
