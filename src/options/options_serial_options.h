#pragma once

#include <boost/program_options/options_description.hpp>

namespace AqualinkAutomate::Options::Serial
{

	boost::program_options::options_description Options();

	void HandleOptions(boost::program_options::variables_map vm);

}
// namespace AqualinkAutomate::Options::Serial
