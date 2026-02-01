#pragma once

#include <cstdint>

#include <boost/circular_buffer.hpp>

namespace AqualinkAutomate::Generators
{

	void BufferCleanUp_ClearBytesFromBeginToPos(boost::circular_buffer<uint8_t>& serial_data, const boost::circular_buffer<uint8_t>::iterator& position);
	void BufferCleanUp_HasEndOfPacketWithinMaxDistance(boost::circular_buffer<uint8_t>& serial_data, const boost::circular_buffer<uint8_t>::iterator& p1s, const boost::circular_buffer<uint8_t>::iterator& p1e, const boost::circular_buffer<uint8_t>::iterator& p2s);

}
// namespace AqualinkAutomate::Generators
