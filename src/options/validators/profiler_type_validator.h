#pragma once

#include <string>
#include <vector>

#include <boost/any.hpp>

#include "profiling/profiler_types.h"

namespace AqualinkAutomate::Options::Validators
{

	// NOTHING HERE

}
// namespace AqualinkAutomate::Options::Validators

namespace AqualinkAutomate::Profiling
{
	void validate(boost::any& v, std::vector<std::string> const& values, ProfilerTypes*, int);
}
// namespace AqualinkAutomate::Profiling
