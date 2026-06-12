#pragma once

#include <string>

#include "messages/jandy_message.h"

namespace AqualinkAutomate::Equipment
{

	// Formats a single RS-485 frame addressed TO the master (destination 0x00) for the
	// "--decode-to-master" developer diagnostic. Observe-only: this only produces a
	// human-readable decode line; it never transmits, emulates the master, or replays.
	//
	// The wire command byte (RawId) is surfaced explicitly because for unrecognised frames
	// the message Id collapses to the catch-all "Unknown" enum and hides the real command.
	// JandyMessage::ToString() supplies the destination, message type and -- for Unknown
	// frames, which now retain their bytes -- the raw payload, which is what lets us reverse
	// engineer commands that other controllers (e.g. an iAqualink2) send to the master.
	std::string FormatToMasterTraffic(const Messages::JandyMessage& message);

}
// namespace AqualinkAutomate::Equipment
