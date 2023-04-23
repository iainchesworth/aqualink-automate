#pragma once

#include <iostream>
#include <string>

#include "profiling/profiler_types.h"

namespace AqualinkAutomate::Profiling
{

	std::istream& operator>>(std::istream& istream, AqualinkAutomate::Profiling::ProfilerTypes& v);

}
// namespace AqualinkAutomate::Profiling
