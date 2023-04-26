#pragma once

#include <iostream>
#include <string>

#include "profiling/types/profiling_types.h"

namespace AqualinkAutomate::Profiling
{

	std::istream& operator>>(std::istream& istream, AqualinkAutomate::Types::ProfilerTypes& v);

}
// namespace AqualinkAutomate::Profiling
