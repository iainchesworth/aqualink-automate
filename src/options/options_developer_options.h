#pragma once

#include <string>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

namespace AqualinkAutomate::Options::Developer
{

	boost::program_options::options_description Options();

	typedef struct
	{
		//bool dev_mode_enabled;
		//std::string dev_serial_port;
		std::string replay_file;
	}
	Settings;

	Settings HandleOptions(boost::program_options::variables_map vm);

}
// namespace AqualinkAutomate::Options::Developer
