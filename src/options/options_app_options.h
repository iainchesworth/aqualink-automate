#pragma once

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

namespace AqualinkAutomate::Options::App
{

	boost::program_options::options_description Options();

	void HandleHelp(boost::program_options::variables_map vm, boost::program_options::options_description& options);
	void HandleOptions(boost::program_options::variables_map vm);
	void HandleVersion(boost::program_options::variables_map vm);
}
// namespace AqualinkAutomate::Options::App
