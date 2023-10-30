#pragma once

#include <string>
#include <utility>
#include <vector>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include "jandy/devices/jandy_device_types.h"
#include "jandy/devices/jandy_emulated_device_types.h"

namespace AqualinkAutomate::Options::Emulated
{

	boost::program_options::options_description Options();

	using JandyEmulatedDevice = std::pair<Devices::JandyEmulatedDeviceTypes, Devices::JandyDeviceType>;
	using JandyEmulatedDeviceCollection = std::vector<JandyEmulatedDevice>;

	typedef struct
	{
		bool disable_emulation;
		JandyEmulatedDeviceCollection emulated_devices{};
	}
	Settings;

	Settings HandleOptions(boost::program_options::variables_map& vm);
	void ValidateOptions(boost::program_options::variables_map& vm);

}
// namespace AqualinkAutomate::Options::Developer
