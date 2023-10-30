#pragma once

#include <string>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

namespace AqualinkAutomate::Options::Developer
{

	boost::program_options::options_description Options();

	typedef struct
	{
		bool debug_logging_enabled{ false };
		bool trace_logging_enabled{ false };
		bool dev_mode_enabled{ False };
		std::string replay_file;
	}
	Settings;

	Settings HandleOptions(boost::program_options::variables_map& vm);
	void ValidateOptions(boost::program_options::variables_map& vm);

}
// namespace AqualinkAutomate::Options::Developer
