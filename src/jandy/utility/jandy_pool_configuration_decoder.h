#pragma once

#include <cstdint>
#include <string>
#include <tuple>
#include <unordered_map>

#include "jandy/config/jandy_config_pool_configurations.h"
#include "jandy/config/jandy_config_system_boards.h"

namespace AqualinkAutomate::Utility
{
	
	class PoolConfigurationDecoder
	{
		using ConfigKey = std::string;
		using ConfigType = std::tuple<Config::PoolConfigurations, Config::SystemBoards, uint8_t, uint8_t>;
		using ConfigMap = std::unordered_map<ConfigKey, ConfigType>;

		static const std::string UNKNOWN_POOL_CONFIG_KEY;
		static const ConfigMap JANDY_EQUIPMENT_CONFIGURATIONS;

	public:
		PoolConfigurationDecoder(const std::string& panel_type);

	public:
		Config::PoolConfigurations Configuration() const;
		Config::SystemBoards SystemBoard() const;
		uint8_t AuxillaryCount() const;
		uint8_t PowerCenterCount() const;

	private:
		static const ConfigType& DecodePanelType(const std::string& panel_type);

	private:
		const ConfigType& m_PoolConfig;
	};	

}
// namespace AqualinkAutomate::Utility
