#pragma once

#include <cstdint>

#include <boost/circular_buffer.hpp>

#include "generator/jandy_message_generator_packetprocessing.h"

namespace AqualinkAutomate::Generators
{

	void BufferCleanUp_ClearBytesFromBeginToPos(boost::circular_buffer<uint8_t>& serial_data, const boost::circular_buffer<uint8_t>::iterator& position);

	// Returns true if the buffer was mutated (bytes erased), false otherwise. Callers can use this
	// to skip the otherwise-mandatory full re-scan of the buffer when no iterators were invalidated.
	bool BufferCleanUp_HasEndOfPacketWithinMaxDistance(boost::circular_buffer<uint8_t>& serial_data, const PacketLocations& locations);

}
// namespace AqualinkAutomate::Generators
