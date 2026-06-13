#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace AqualinkAutomate::Kernel
{

	// Result of cross-checking the discovered equipment set against the model's expected
	// layout (aux count + power-centre count from PoolConfigurationDecoder). Populated once
	// the startup scrape completes; surfaced via the equipment API so a mis-wired panel or
	// an incomplete/short scrape is visible rather than silently accepted.
	struct EquipmentValidation
	{
		uint8_t ExpectedAuxillaries{ 0 };
		uint8_t DiscoveredAuxillaries{ 0 };   // numbered aux relays + DIP-reconfigured aux-relay equipment
		uint8_t ExpectedPowerCenters{ 0 };
		uint8_t DiscoveredPowerCenters{ 0 };
		std::vector<std::string> Anomalies;   // human-readable issues; empty == validated clean

		bool Passed() const { return Anomalies.empty(); }
	};

}
// namespace AqualinkAutomate::Kernel
