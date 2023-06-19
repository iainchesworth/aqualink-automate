#pragma once

#include <string>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include "jandy/devices/jandy_device_types.h"
#include "jandy/devices/jandy_emulated_device_types.h"

namespace AqualinkAutomate::Options::Emulated
{

	boost::program_options::options_description Options();

	typedef struct
	{
		bool disable_emulation;
		Devices::JandyEmulatedDeviceTypes controller_type;
		Devices::JandyDeviceType device_type;
	}
	Settings;

	Settings HandleOptions(boost::program_options::variables_map& vm);
	void ValidateOptions(boost::program_options::variables_map& vm);

}
// namespace AqualinkAutomate::Options::Developer
