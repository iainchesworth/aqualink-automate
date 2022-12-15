#pragma once

#include <boost/program_options/options_description.hpp>

namespace AqualinkAutomate::Options::Web
{

	boost::program_options::options_description Options();

	void HandleOptions(boost::program_options::variables_map vm);

}
// namespace AqualinkAutomate::Options::Web
