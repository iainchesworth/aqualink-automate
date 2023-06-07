#pragma once

#include <string>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

namespace AqualinkAutomate::Options::Serial
{

	boost::program_options::options_description Options();

	typedef struct
	{
		std::string serial_port;
	}
	Settings;

	Settings HandleOptions(boost::program_options::variables_map& vm);
	void ValidateOptions(boost::program_options::variables_map& vm);

}
// namespace AqualinkAutomate::Options::Serial
