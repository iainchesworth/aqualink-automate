#pragma once

#include <string>
#include <vector>

#include <boost/any.hpp>

#include "jandy/devices/jandy_emulated_device_types.h"

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
//     -> AqualinkAutomate::Devices for JandyEmulatedDeviceTypes
//
//=============================================================================

namespace AqualinkAutomate::Devices
{
	void validate(boost::any& v, std::vector<std::string> const& values, JandyEmulatedDeviceTypes*, int);
}
// namespace AqualinkAutomate::Devices
