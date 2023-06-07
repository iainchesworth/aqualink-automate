#pragma once

#include <boost/program_options/variables_map.hpp>

#include "options/options_option_type.h"

namespace AqualinkAutomate::Options
{

	void Helper_ValidateOptionDependencies(const boost::program_options::variables_map& vm, const AppOptionPtr& for_what, const AppOptionPtr& required_option);

}
// namespace AqualinkAutomate::Options
