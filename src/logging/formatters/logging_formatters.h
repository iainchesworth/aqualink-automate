#pragma once

#include <iostream>
#include <string>

#include "logging/logging_severity_levels.h"

namespace AqualinkAutomate::Logging
{

	std::istream& operator>>(std::istream& istream, AqualinkAutomate::Logging::Severity& v);

}
// namespace AqualinkAutomate::Logging
