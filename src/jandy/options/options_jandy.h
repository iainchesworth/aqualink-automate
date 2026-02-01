#pragma once

#include <expected>
#include <string>
#include <vector>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>

#include "devices/jandy_device_types.h"
#include "devices/jandy_emulated_device_types.h"
#include "errors/options_errors.h"
#include "options/options_option_type.h"
#include "options/validators/jandy_device_id_validator.h"
#include "options/validators/jandy_emulated_device_type_validator.h"

using namespace AqualinkAutomate::Options;

namespace AqualinkAutomate::Jandy::Options
{

	using JandyEmulatedDevice = std::pair<Devices::JandyEmulatedDeviceTypes, Devices::JandyDeviceType>;
	using JandyEmulatedDeviceCollection = std::vector<JandyEmulatedDevice>;

	typedef struct tagJandySettings
	{
		static const std::string& AreaName()
		{
			static const std::string AREA_NAME{ "Jandy" };
			return AREA_NAME;
		}

		tagJandySettings() :
			disable_emulation{ false },
			emulated_devices{}
		{
		}

		bool disable_emulation;
		JandyEmulatedDeviceCollection emulated_devices;
	}
	JandySettings;

	class OptionsProcessor
	{
	private:
		AppOptionPtr OPTION_DISABLEEMULATION{ make_appoption("disable-emulation", "Disable Aqualink controller emulation", boost::program_options::bool_switch()->default_value(false)) };
		AppOptionPtr OPTION_EMULATEDDEVICETYPE{ make_appoption("device-type", "Controller emulation type", boost::program_options::value<std::vector<Devices::JandyEmulatedDeviceTypes>>()->multitoken()) };
		AppOptionPtr OPTION_EMULATEDDEVICEID{ make_appoption("device-id", "Controller serial id", boost::program_options::value<std::vector<Devices::JandyDeviceId>>()->multitoken()) };

		std::vector<AppOptionPtr> JandyOptionsCollection
		{
			OPTION_DISABLEEMULATION,
			OPTION_EMULATEDDEVICETYPE,
			OPTION_EMULATEDDEVICEID
		};

	public:
		using SettingsType = JandySettings;

	public:
		std::string Name() const { return SettingsType::AreaName(); }
		boost::program_options::options_description Options() const;

	public:
		void Validate(const boost::program_options::variables_map& vm) const;
		std::expected<SettingsType, ErrorCodes::Options_ErrorCodes> Process(boost::program_options::variables_map& vm) const;
	};

}
// namespace AqualinkAutomate::Jandy::Options
