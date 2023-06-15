#include <algorithm>
#include <array>
#include <format>

#include <magic_enum.hpp>

#include "jandy/config/jandy_config_powercenter.h"

namespace AqualinkAutomate::Config
{

	PowerCenters::PowerCenters(const uint8_t total_auxillary_count) :
		m_PC_A(),
		m_RPC_B(PowerCenter_NotInstalled()),
		m_RPC_C(PowerCenter_NotInstalled()),
		m_RPC_D(PowerCenter_NotInstalled()),
		m_NonExistentRPC()
	{
		AllocateAuxillaries(total_auxillary_count);
	}

	PowerCenter& PowerCenters::LocalPowerCenter()
	{
		return m_PC_A;
	}

	bool PowerCenters::IsRemotePowerCenterInstalled(const RemotePowerCenterIds& rpc_id)
	{
		switch (rpc_id)
		{
		case RemotePowerCenterIds::B: return std::holds_alternative<PowerCenter>(m_RPC_B);
		case RemotePowerCenterIds::C: return std::holds_alternative<PowerCenter>(m_RPC_C);
		case RemotePowerCenterIds::D: return std::holds_alternative<PowerCenter>(m_RPC_D);
		default: return false;
		}
	}

	PowerCenter& PowerCenters::RemotePowerCenter(const RemotePowerCenterIds& rpc_id)
	{
		switch (rpc_id)
		{
		case RemotePowerCenterIds::B: return IsRemotePowerCenterInstalled(rpc_id) ? std::get<PowerCenter>(m_RPC_B) : m_NonExistentRPC;
		case RemotePowerCenterIds::C: return IsRemotePowerCenterInstalled(rpc_id) ? std::get<PowerCenter>(m_RPC_C) : m_NonExistentRPC;
		case RemotePowerCenterIds::D: return IsRemotePowerCenterInstalled(rpc_id) ? std::get<PowerCenter>(m_RPC_D) : m_NonExistentRPC;
		default: return m_NonExistentRPC;
		}
	}

	void PowerCenters::AllocateAuxillaries(uint8_t total_auxillary_count)
	{
		const uint8_t TOTAL_POWER_CENTERS = 4;
		
		std::array<uint8_t, TOTAL_POWER_CENTERS> MaxAuxPerPowerCenter = { 7, 8, 8, 8 };
		std::array<uint8_t, TOTAL_POWER_CENTERS> AuxPerPowerCenter = { 0, 0, 0, 0 };

		for (auto i = 0; i < TOTAL_POWER_CENTERS && total_auxillary_count > 0; ++i)
		{
			AuxPerPowerCenter[i] = std::min(total_auxillary_count, MaxAuxPerPowerCenter[i]);
			total_auxillary_count -= AuxPerPowerCenter[i];
		}

		auto add_aux_to_powercenter = [](auto& pc, uint8_t aux_count_to_add) -> void
		{
			if (0 == aux_count_to_add)
			{
				// DO NOTHING
			}
			else
			{
				for (auto i = 0; i < aux_count_to_add; ++i)
				{
					const std::string aux_name = std::format("Aux{}", i + 1);
					pc.push_back(Auxillary{ aux_name });
				}
			}
		};

		auto add_aux_to_remotepowercenter = [&](auto& rpc, RemotePowerCenterIds rpc_id, uint8_t aux_count_to_add) -> void
		{
			if (0 == aux_count_to_add)
			{
				// DO NOTHING
			}
			else
			{
				if (!IsRemotePowerCenterInstalled(rpc_id))
				{
					rpc = PowerCenter();
				}
				else
				{
					std::get<PowerCenter>(rpc).clear();
				}

				for (auto i = 0; i < aux_count_to_add; ++i)
				{
					const std::string aux_name = std::format("Aux {}{}", magic_enum::enum_name(rpc_id), i + 1);
					std::get<PowerCenter>(rpc).emplace_back(Auxillary{ aux_name });
				}
			}
		};

		add_aux_to_powercenter(m_PC_A, AuxPerPowerCenter[0]);
		add_aux_to_remotepowercenter(m_RPC_B, RemotePowerCenterIds::B, AuxPerPowerCenter[1]);
		add_aux_to_remotepowercenter(m_RPC_C, RemotePowerCenterIds::C, AuxPerPowerCenter[2]);
		add_aux_to_remotepowercenter(m_RPC_D, RemotePowerCenterIds::D, AuxPerPowerCenter[3]);
	}

}
// namespace AqualinkAutomate::Config
