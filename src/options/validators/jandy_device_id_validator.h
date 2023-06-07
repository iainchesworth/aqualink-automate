#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <boost/any.hpp>

#include "jandy/devices/jandy_device_id.h"

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
//     -> AqualinkAutomate::Devices for JandyDeviceId
//
//=============================================================================

namespace AqualinkAutomate::Devices
{

	void validate(boost::any& v, std::vector<std::string> const& values, JandyDeviceId*, int);

}
// namespace AqualinkAutomate::Devices
