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

		{"RS-4 Only", {Kernel::PoolConfigurations::SingleBody, Kernel::SystemBoards::RS4_Only, 3, 1}},
		{"RS-6 Only", {Kernel::PoolConfigurations::SingleBody, Kernel::SystemBoards::RS6_Only, 5, 1}},
		{"RS-8 Only", {Kernel::PoolConfigurations::SingleBody, Kernel::SystemBoards::RS8_Only, 7, 1}},
		{"PD-4 Only", {Kernel::PoolConfigurations::SingleBody, Kernel::SystemBoards::PD4_Only, 3, 1}},
		{"PD-8 Only", {Kernel::PoolConfigurations::SingleBody, Kernel::SystemBoards::PD8_Only, 7, 1}},

		// DUAL BODY, SHARED EQUIPMENT

		{"RS-4 Combo",  {Kernel::PoolConfigurations::DualBody_SharedEquipment, Kernel::SystemBoards::RS4_Combo,  3,  1}},
		{"RS-6 Combo",  {Kernel::PoolConfigurations::DualBody_SharedEquipment, Kernel::SystemBoards::RS6_Combo,  5,  1}},
		{"RS-8 Combo",  {Kernel::PoolConfigurations::DualBody_SharedEquipment, Kernel::SystemBoards::RS8_Combo,  7,  1}},
		{"RS-12 Combo", {Kernel::PoolConfigurations::DualBody_SharedEquipment, Kernel::SystemBoards::RS12_Combo, 11, 2}},
		{"RS-16 Combo", {Kernel::PoolConfigurations::DualBody_SharedEquipment, Kernel::SystemBoards::RS16_Combo, 15, 2}},
		{"RS-24 Combo", {Kernel::PoolConfigurations::DualBody_SharedEquipment, Kernel::SystemBoards::RS24_Combo, 23, 3}},
		{"RS-32 Combo", {Kernel::PoolConfigurations::DualBody_SharedEquipment, Kernel::SystemBoards::RS32_Combo, 31, 4}},
		{"PD-4 Combo",  {Kernel::PoolConfigurations::DualBody_SharedEquipment, Kernel::SystemBoards::PD4_Combo,  3,  1}},
		{"PD-6 Combo",  {Kernel::PoolConfigurations::DualBody_SharedEquipment, Kernel::SystemBoards::PD6_Combo,  5,  1}},
		{"PD-8 Combo",  {Kernel::PoolConfigurations::DualBody_SharedEquipment, Kernel::SystemBoards::PD8_Combo,  7,  1}},

		// DUAL BODY, DUAL EQUIPMENT

		{"RS-2/6 Dual",  {Kernel::PoolConfigurations::DualBody_DualEquipment, Kernel::SystemBoards::RS2_6_Dual,  6,  1}},
		{"RS-2/10 Dual", {Kernel::PoolConfigurations::DualBody_DualEquipment, Kernel::SystemBoards::RS2_10_Dual, 10, 2}},
		{"RS-2/14 Dual", {Kernel::PoolConfigurations::DualBody_DualEquipment, Kernel::SystemBoards::RS2_14_Dual, 14, 2}},
		{"RS-2/22 Dual", {Kernel::PoolConfigurations::DualBody_DualEquipment, Kernel::SystemBoards::RS2_22_Dual, 22, 3}},
		{"RS-2/30 Dual", {Kernel::PoolConfigurations::DualBody_DualEquipment, Kernel::SystemBoards::RS2_30_Dual, 30, 4}},

		// UNKNOWN EQUIPMENT

		{UNKNOWN_POOL_CONFIG_KEY, {Kernel::PoolConfigurations::Unknown, Kernel::SystemBoards::Unknown, 1, 1}}
	};

	PoolConfigurationDecoder::PoolConfigurationDecoder(const std::string& panel_type) :
		m_PoolConfig(DecodePanelType(panel_type))
	{		
	}

	Kernel::PoolConfigurations PoolConfigurationDecoder::Configuration() const
	{
		return std::get<Kernel::PoolConfigurations>(m_PoolConfig);
	}

	Kernel::SystemBoards PoolConfigurationDecoder::SystemBoard() const
	{
		return std::get<Kernel::SystemBoards>(m_PoolConfig);
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
