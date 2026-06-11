#pragma once

#include <optional>
#include <string>

namespace AqualinkAutomate::Jandy::Startup
{

	// Feature support gated by the AquaLink RS *software* revision letter, per Jandy's published
	// revision history (TroubleFreePool wiki "Jandy Aqualink RS" + the "AquaLink RS Software and
	// PCB Revisions" sheet + the "Jandy Aqualink PPD/CPU Revisions" thread). See
	// docs/aqualink_rs_revisions.md for the full table and sources.
	//
	// The panel reports this as e.g. "REV T.0.1"; the leading letter is the major revision.
	// Revisions A-M are PPD-architecture boards; Rev N (2007) replaced the PPD with a CPU board,
	// and later revisions added device/protocol support:
	//
	//   Rev N (2007)  CPU board (PPD -> CPU); RS-485 to AE Heat Pump / LXi Gas Heater
	//   Rev O (2008)  Variable-Speed pump communication
	//   Rev P (2009)  ChemLink, LM3, AutoClear Plus & DuoClear chlorinators
	//   Rev Q (2010)  Touch Screen panel (AquaLink Touch -- the 0x33 page protocol)
	//   Rev R (2012)  iAquaLink internet/cloud control (0xa3)
	//   Rev W (2022)  up to 16 address-assigned Variable-Speed pumps
	//   Rev Y         Jandy Infinite WaterColors LED light controller
	//
	// (Pre-N PPD *chip* revisions use a separate C..HH / 44-vs-52-pin-socket scheme; that is a
	// hardware-fit concern, not a protocol capability, so it is not modelled here.)
	struct RevisionCapabilities
	{
		char revision_letter{ '\0' };          // major software revision; '\0' if unparsed
		bool is_known{ false };                // a revision letter was parsed

		bool cpu_board{ false };               // Rev N+  (else PPD architecture)
		bool variable_speed_pumps{ false };    // Rev O+
		bool chemlink_chlorinators{ false };   // Rev P+  (ChemLink, LM3, AutoClear Plus, DuoClear)
		bool aqualink_touch{ false };          // Rev Q+  (the AqualinkTouch page protocol)
		bool iaqualink_cloud{ false };         // Rev R+
		bool addressed_vs_pumps{ false };      // Rev W+
		bool infinite_watercolors{ false };    // Rev Y+
	};

	// Extract the major revision letter from a reported revision string ("REV T.0.1" -> 'T',
	// "Rev O" -> 'O', "o" -> 'O'). Skips a leading "REV" token. Returns nullopt when no A-Z
	// major-revision letter is present.
	std::optional<char> ParseRevisionLetter(const std::string& revision);

	// Map a revision string to its capability set. An unparseable/empty string yields an
	// all-false set with is_known == false (the caller then leans on observed bus behaviour).
	RevisionCapabilities DeriveRevisionCapabilities(const std::string& revision);

}
// namespace AqualinkAutomate::Jandy::Startup
