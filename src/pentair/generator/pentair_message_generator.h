#pragma once

#include <cstdint>

#include <boost/circular_buffer.hpp>

#include "types/pentair_types.h"

namespace AqualinkAutomate::Pentair::Generators
{

	// Attempt to extract a single Pentair frame from the front of the serial
	// buffer.
	//
	// PROTOCOL DETECTION / COEXISTENCE: this generator only ever consumes bytes
	// it can positively attribute to the Pentair wire format (a 0xFF 0x00 0xFF
	// 0xA5 preamble).  If no Pentair preamble is present it returns
	// WaitingForMoreData WITHOUT touching the buffer, so a co-registered Jandy
	// generator (which frames on DLE/STX) can still claim the same bytes.  Bytes
	// ahead of a located preamble are discarded only once a preamble is found.
	[[nodiscard]] Types::PentairExpectedMessageType GenerateMessageFromRawData(boost::circular_buffer<uint8_t>& serial_data);

}
// namespace AqualinkAutomate::Pentair::Generators
