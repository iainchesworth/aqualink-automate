#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "devices/jandy_emulated_device_types.h"
#include "startup/jandy_revision_capabilities.h"

namespace AqualinkAutomate::Jandy::Startup
{

	// How the app sources live state from the panel once the controller type is known.
	// Each value maps to an existing device/engine already in the codebase.
	enum class DataGatheringMethod
	{
		Unknown,
		PagePush,     // AqualinkTouch (0x30-0x33): the master PUSHES MainStatus/AuxStatus/pages,
		              // and we navigate to specific pages on demand (SelectPageButton). No crawl.
		MenuSpider,   // OneTouch (0x40-0x43): autonomous menu crawl (Navigator/SpiderEngine).
		PdaGraph,     // PDA (0x60-0x63): pre-computed ScreenDataPageGraph walk (Scrapeable).
		ObserveOnly,  // No emulatable controller slot probed: passive decode only.
	};

	// Identity + capabilities inferred about the panel ("PowerCenter type and revision").
	// Capabilities are taken from the master's discovery PROBE set (which address ranges it
	// scans at boot) -- the robust, observable signal -- with model/revision as identity
	// metadata sourced separately (SerialAdapter model command, or a OneTouch/PDA scrape).
	struct PanelProfile
	{
		std::string model;       // e.g. "PD-8 Combo" (may be empty until sourced)
		std::string revision;    // firmware revision (may be empty until sourced)

		bool probes_aqualinktouch{ false };  // master probes 0x30-0x33
		bool probes_onetouch{ false };       // master probes 0x40-0x43
		bool probes_pda{ false };            // master probes 0x60-0x63
		bool has_iaqualink2_slot{ false };   // master probes 0xa3 (a configured cloud iface)

		// Feature support implied by the panel's software revision (e.g. Rev Q+ -> AqualinkTouch,
		// Rev O+ -> VS pumps). Derived from `revision`; cross-checks the observed probe set and
		// tells downstream which peripherals to expect. is_known is false until revision is sourced.
		RevisionCapabilities revision_caps;
	};

	// A controller/peripheral the plan wants to emulate, with preference-ordered candidate
	// bus addresses. Address selection picks the first candidate NOT already answered by a
	// real device, so an emulated controller never collides with hardware on the bus.
	struct PlannedDevice
	{
		Devices::JandyEmulatedDeviceTypes type{ Devices::JandyEmulatedDeviceTypes::Unknown };
		std::vector<std::uint8_t> candidate_ids;     // preference-ordered
		std::uint8_t selected_id{ 0x00 };            // resolved bus id (0x00 == none free)
		bool resolved{ false };                      // true once an address was selected
		std::string role;                            // human note for logs / diagnostics
	};

	// The decision: how to source data + which devices to stand up (and where).
	struct StartupPlan
	{
		DataGatheringMethod method{ DataGatheringMethod::Unknown };
		std::vector<PlannedDevice> devices;
		std::string rationale;

		// Revision-implied capabilities carried through for downstream consumers (which
		// peripherals to expect/decode: VS pumps, ChemLink/AutoClear chlorinators, etc.).
		RevisionCapabilities revision_caps;

		// True when the observed probe set and the revision-implied capabilities AGREE about
		// AqualinkTouch support; false flags a discrepancy worth logging (mis-decode, unusual
		// config, or a revision we don't recognise). Only meaningful when revision_caps.is_known.
		bool revision_consistent{ true };
	};

}
// namespace AqualinkAutomate::Jandy::Startup
