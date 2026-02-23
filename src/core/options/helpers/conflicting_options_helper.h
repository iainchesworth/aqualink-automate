#pragma once

#include <boost/program_options/variables_map.hpp>

#include "options/options_option_type.h"

namespace AqualinkAutomate::Options
{

	void Helper_CheckForConflictingOptions(const boost::program_options::variables_map& vm, const AppOptionPtr& opt1, const AppOptionPtr& opt2);

}
// namespace AqualinkAutomate::Options
