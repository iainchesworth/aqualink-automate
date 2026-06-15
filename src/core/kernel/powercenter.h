#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace AqualinkAutomate::Kernel
{

	// The Jandy AquaLink RS supports up to four power centers (relay banks): a local
	// one (A) and up to three remote ones (B/C/D). Auxillary relays are numbered per
	// centre -- "Aux1".."Aux7" on A, "Aux B1".."Aux B8" on B, and so on.
	enum class PowerCenterIds : uint8_t
	{
		A = 0,
		B = 1,
		C = 2,
		D = 3
	};

	inline constexpr uint8_t MAX_POWER_CENTERS = 4;

	// Per-centre maximum aux capacity (local A holds 7, each remote B/C/D holds 8).
	// Confirmed live against the Alwin32 simulator (RS-16 Combo => A=7 + B=8 = 15 aux).
	inline constexpr std::array<uint8_t, MAX_POWER_CENTERS> POWER_CENTER_AUX_CAPACITY{ 7, 8, 8, 8 };

	// Attribution container: discovered auxillaries are sorted into their power centre.
	// Membership derives from the scraped aux id/label, NOT from a count -- dual-equipment
	// models split relays unevenly across centres (e.g. RS-2/10 is A=6 + B=4, not A=7 + B=3)
	// and the IO-board DIP switches can repurpose an individual relay (e.g. Aux3 -> Cleaner),
	// so a count-based allocation cannot reconstruct the real layout.
	class PowerCenters
	{
	public:
		// Attribute one discovered auxillary (by label) to a power centre.
		void Assign(PowerCenterIds pc, const std::string& aux_label);

		const std::vector<std::string>& AuxillariesIn(PowerCenterIds pc) const;

		// A centre is "installed" once at least one auxillary has been attributed to it.
		bool IsInstalled(PowerCenterIds pc) const;

		// Number of centres with at least one auxillary attributed.
		uint8_t InstalledCount() const;

		// Total auxillaries attributed across all centres.
		uint8_t TotalAuxillaries() const;

	private:
		std::array<std::vector<std::string>, MAX_POWER_CENTERS> m_Centers;
	};

}
// namespace AqualinkAutomate::Kernel
