#include <format>

#include "jandy/utility/jandy_pool_configuration_decoder.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Utility
{
	const std::string PoolConfigurationDecoder::UNKNOWN_POOL_CONFIG_KEY{"UNKNOWN POOL CONFIG"};

	const PoolConfigurationDecoder::ConfigMap PoolConfigurationDecoder::JANDY_EQUIPMENT_CONFIGURATIONS
	{
		// SINGLE BODY ONLY

		{"RS-4 Only", {Config::PoolConfigurations::SingleBody, Config::SystemBoards::RS4_Only, 3, 1}},
		{"RS-6 Only", {Config::PoolConfigurations::SingleBody, Config::SystemBoards::RS6_Only, 5, 1}},
		{"RS-8 Only", {Config::PoolConfigurations::SingleBody, Config::SystemBoards::RS8_Only, 7, 1}},
		{"PD-4 Only", {Config::PoolConfigurations::SingleBody, Config::SystemBoards::PD4_Only, 3, 1}},
		{"PD-8 Only", {Config::PoolConfigurations::SingleBody, Config::SystemBoards::PD8_Only, 7, 1}},

		// DUAL BODY, SHARED EQUIPMENT

		{"RS-4 Combo",  {Config::PoolConfigurations::DualBody_SharedEquipment, Config::SystemBoards::RS4_Combo,  3,  1}},
		{"RS-6 Combo",  {Config::PoolConfigurations::DualBody_SharedEquipment, Config::SystemBoards::RS6_Combo,  5,  1}},
		{"RS-8 Combo",  {Config::PoolConfigurations::DualBody_SharedEquipment, Config::SystemBoards::RS8_Combo,  7,  1}},
		{"RS-12 Combo", {Config::PoolConfigurations::DualBody_SharedEquipment, Config::SystemBoards::RS12_Combo, 11, 2}},
		{"RS-16 Combo", {Config::PoolConfigurations::DualBody_SharedEquipment, Config::SystemBoards::RS16_Combo, 15, 2}},
		{"RS-24 Combo", {Config::PoolConfigurations::DualBody_SharedEquipment, Config::SystemBoards::RS24_Combo, 23, 3}},
		{"RS-32 Combo", {Config::PoolConfigurations::DualBody_SharedEquipment, Config::SystemBoards::RS32_Combo, 31, 4}},
		{"PD-4 Combo",  {Config::PoolConfigurations::DualBody_SharedEquipment, Config::SystemBoards::PD4_Combo,  3,  1}},
		{"PD-6 Combo",  {Config::PoolConfigurations::DualBody_SharedEquipment, Config::SystemBoards::PD6_Combo,  5,  1}},
		{"PD-8 Combo",  {Config::PoolConfigurations::DualBody_SharedEquipment, Config::SystemBoards::PD8_Combo,  7,  1}},

		// DUAL BODY, DUAL EQUIPMENT

		{"RS-2/6 Dual",  {Config::PoolConfigurations::DualBody_DualEquipment, Config::SystemBoards::RS2_6_Dual,  6,  1}},
		{"RS-2/10 Dual", {Config::PoolConfigurations::DualBody_DualEquipment, Config::SystemBoards::RS2_10_Dual, 10, 2}},
		{"RS-2/14 Dual", {Config::PoolConfigurations::DualBody_DualEquipment, Config::SystemBoards::RS2_14_Dual, 14, 2}},
		{"RS-2/22 Dual", {Config::PoolConfigurations::DualBody_DualEquipment, Config::SystemBoards::RS2_22_Dual, 22, 3}},
		{"RS-2/30 Dual", {Config::PoolConfigurations::DualBody_DualEquipment, Config::SystemBoards::RS2_30_Dual, 30, 4}},

		// UNKNOWN EQUIPMENT

		{UNKNOWN_POOL_CONFIG_KEY, {Config::PoolConfigurations::Unknown, Config::SystemBoards::Unknown, 1, 1}}
	};

	PoolConfigurationDecoder::PoolConfigurationDecoder(const std::string& panel_type) :
		m_PoolConfig(DecodePanelType(panel_type))
	{		
	}

	Config::PoolConfigurations PoolConfigurationDecoder::Configuration() const
	{
		return std::get<Config::PoolConfigurations>(m_PoolConfig);
	}

	Config::SystemBoards PoolConfigurationDecoder::SystemBoard() const
	{
		return std::get<Config::SystemBoards>(m_PoolConfig);
	}

	uint8_t PoolConfigurationDecoder::AuxillaryCount() const
	{
		return std::get<2>(m_PoolConfig);
	}

	uint8_t PoolConfigurationDecoder::PowerCenterCount() const
	{
		return std::get<3>(m_PoolConfig);
	}

	const PoolConfigurationDecoder::ConfigType& PoolConfigurationDecoder::DecodePanelType(const std::string& panel_type)
	{
		if (auto it = JANDY_EQUIPMENT_CONFIGURATIONS.find(panel_type); it == JANDY_EQUIPMENT_CONFIGURATIONS.end())
		{
			LogDebug(Channel::Equipment, std::format("Cannot decode pool configuration; unknown panel type -> {}", panel_type));
		}
		else
		{
			return JANDY_EQUIPMENT_CONFIGURATIONS.at(panel_type);
		}

		return JANDY_EQUIPMENT_CONFIGURATIONS.at(UNKNOWN_POOL_CONFIG_KEY);
	}

}
// namespace AqualinkAutomate::Utility
