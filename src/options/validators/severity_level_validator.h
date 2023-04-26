#pragma once

#include <string>
#include <vector>

#include <boost/any.hpp>

#include "logging/logging_severity_levels.h"

namespace AqualinkAutomate::Options::Validators
{

	// NOTHING HERE
	
}
// namespace AqualinkAutomate::Options::Validators

//=============================================================================
//
// BOOST Program Options requires validators to be in the namespace of the
// type or entry being validated.
// 
//     -> AqualinkAutomate::Logging for Severity
//
//=============================================================================

namespace AqualinkAutomate::Logging
{
	void validate(boost::any& v, std::vector<std::string> const& values, Severity*, int);
}
// namespace AqualinkAutomate::Logging
