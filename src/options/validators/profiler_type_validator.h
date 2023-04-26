#pragma once

#include <string>
#include <vector>

#include <boost/any.hpp>

#include "profiling/types/profiling_types.h"

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
//     -> AqualinkAutomate::Types for ProfilerTypes
//
//=============================================================================

namespace AqualinkAutomate::Types
{
	void validate(boost::any& v, std::vector<std::string> const& values, ProfilerTypes*, int);
}
// namespace AqualinkAutomate::Types
