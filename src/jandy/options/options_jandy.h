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

using namespace AqualinkAutomate::Options;

namespace AqualinkAutomate::Jandy::Options
{

	using JandyEmulatedDevice = std::pair<Devices::JandyEmulatedDeviceTypes, Devices::JandyDeviceType>;
	using JandyEmulatedDeviceCollection = std::vector<JandyEmulatedDevice>;

	struct JandySettings
	{
		static const std::string& AreaName()
		{
			static const std::string AREA_NAME{ "Jandy" };
			return AREA_NAME;
		}

		JandySettings() :
			disable_emulation{ false },
			emulated_devices{},
			navigation_password{}
		{
		}

		bool disable_emulation;
		JandyEmulatedDeviceCollection emulated_devices;
		std::string navigation_password;  // 4-digit password for menu navigation
	};

	class OptionsProcessor
	{
	private:
		AppOptionPtr OPTION_DISABLEEMULATION{ make_appoption("jandy-disable-emulation", "Disable Jandy controller emulation", boost::program_options::bool_switch()->default_value(false)) };
		AppOptionPtr OPTION_EMULATEDDEVICETYPE{ make_appoption("jandy-device-type", "Comma-separated Jandy emulation types (e.g. iaq,onetouch)", boost::program_options::value<std::string>()) };
		AppOptionPtr OPTION_EMULATEDDEVICEID{ make_appoption("jandy-device-id", "Comma-separated hex device IDs (e.g. 0xa1,0x41)", boost::program_options::value<std::string>()) };
		AppOptionPtr OPTION_NAVPASSWORD{ make_appoption("jandy-nav-password", "4-digit password for navigating Jandy password-protected menus", boost::program_options::value<std::string>()->default_value("")) };

		const std::vector<AppOptionPtr> JandyOptionsCollection
		{
			OPTION_DISABLEEMULATION,
			OPTION_EMULATEDDEVICETYPE,
			OPTION_EMULATEDDEVICEID,
			OPTION_NAVPASSWORD
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
