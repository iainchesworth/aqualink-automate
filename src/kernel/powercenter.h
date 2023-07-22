#pragma once

#include <cstdint>
#include <variant>
#include <vector>

#include "kernel/auxillary_devices/auxillary.h"

namespace AqualinkAutomate::Kernel
{

	enum class RemotePowerCenterIds
	{
		B,
		C,
		D
	};

	struct PowerCenter_NotInstalled {};

	using PowerCenter = std::vector<Auxillary>;
	using RemotePowerCenter = std::variant<PowerCenter_NotInstalled, PowerCenter>;
	
	class PowerCenters
	{		
	public:
		PowerCenters(const uint8_t total_auxillary_count);

	public:
		PowerCenter& LocalPowerCenter();

	private:
		PowerCenter m_PC_A;

	public:
		bool IsRemotePowerCenterInstalled(const RemotePowerCenterIds& rpc_id);
		PowerCenter& RemotePowerCenter(const RemotePowerCenterIds& id);

	private:
		void AllocateAuxillaries(uint8_t total_auxillary_count);

	private:
		std::variant<PowerCenter_NotInstalled, PowerCenter> m_RPC_B;
		std::variant<PowerCenter_NotInstalled, PowerCenter> m_RPC_C;
		std::variant<PowerCenter_NotInstalled, PowerCenter> m_RPC_D;
		PowerCenter m_NonExistentRPC;
	};

}
// namespace AqualinkAutomate::Kernel
