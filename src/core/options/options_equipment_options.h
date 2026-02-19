#pragma once

#include <expected>
#include <string>
#include <vector>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include "errors/options_errors.h"
#include "kernel/pool_configurations.h"
#include "options/options_option_type.h"

namespace AqualinkAutomate::Options::Equipment
{

	typedef struct tagEquipmentSettings
	{
		static const std::string& AreaName()
		{
			static const std::string AREA_NAME{ "Equipment" };
			return AREA_NAME;
		}

		tagEquipmentSettings() :
			pool_configuration{ Kernel::PoolConfigurations::Unknown },
			pool_configuration_is_user_specified{ false }
		{
		}

		Kernel::PoolConfigurations pool_configuration;
		bool pool_configuration_is_user_specified;
	}
	EquipmentSettings;

	class OptionsProcessor
	{
	private:
		AppOptionPtr OPTION_POOL_CONFIGURATION{ make_appoption("pool-configuration", "Pool installation type: pool-only, spa-only, combo, dual (default: auto)", boost::program_options::value<std::string>()->default_value("auto")) };

		const std::vector<AppOptionPtr> EquipmentOptionsCollection
		{
			OPTION_POOL_CONFIGURATION
		};

	public:
		using SettingsType = EquipmentSettings;

	public:
		std::string Name() const { return SettingsType::AreaName(); }
		boost::program_options::options_description Options() const;

	public:
		void Validate(const boost::program_options::variables_map& vm) const;
		std::expected<SettingsType, ErrorCodes::Options_ErrorCodes> Process(boost::program_options::variables_map& vm) const;
	};

}
// namespace AqualinkAutomate::Options::Equipment
