#pragma once

#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "startup/jandy_startup_types.h"

namespace AqualinkAutomate::Jandy::Startup
{

	// Pure decision logic for emulation start-up: no I/O, no hubs. Everything is computed
	// from observed bus behaviour + identity strings, so it is fully unit-testable without
	// hardware. The coordinator (jandy_startup_coordinator) drives this against the live app.
	class StartupPlanner
	{
	public:
		// Candidate bus addresses, most-preferred first. 0x33 is the canonical AqualinkTouch
		// id and 0x41 the canonical OneTouch id; the lower instances are relocation fallbacks
		// used when a real device already occupies the preferred slot.
		static const std::vector<std::uint8_t> AQUALINKTOUCH_IDS;
		static const std::vector<std::uint8_t> ONETOUCH_IDS;
		static const std::vector<std::uint8_t> PDA_IDS;
		static const std::vector<std::uint8_t> SERIALADAPTER_IDS;

	public:
		// Infer panel capabilities from the master's discovery probe set + identity strings.
		static PanelProfile DeriveProfile(const std::set<std::uint8_t>& probed_addresses,
			const std::string& model, const std::string& revision);

		// Choose the data-gathering method + the emulated device set (with candidate ids). A
		// SerialAdapter is always included (panel model + command channel); the controller
		// choice prefers page-push (AqualinkTouch) over menu spidering (OneTouch) over PDA.
		static StartupPlan Plan(const PanelProfile& profile);

		// First candidate not present in `occupied`; nullopt if every candidate is taken.
		static std::optional<std::uint8_t> SelectFreeAddress(
			const std::vector<std::uint8_t>& candidates, const std::set<std::uint8_t>& occupied);

		// Resolve every device's address against `occupied` (real devices answering on the
		// bus), claiming each chosen id so two planned devices never collide. A device whose
		// every candidate is taken is left unresolved (selected_id 0x00) for the caller to
		// skip. This implements "confirm 0x33 is free, else stand up at a second slot".
		static void ResolveAddresses(StartupPlan& plan, const std::set<std::uint8_t>& occupied);
	};

}
// namespace AqualinkAutomate::Jandy::Startup
