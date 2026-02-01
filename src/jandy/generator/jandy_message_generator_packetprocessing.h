#pragma once

#include <cstdint>
#include <span>

#include <boost/circular_buffer.hpp>

namespace AqualinkAutomate::Generators
{

	void PacketProcessing_GetPacketLocations(boost::circular_buffer<uint8_t>& serial_data, boost::circular_buffer<uint8_t>::iterator& p1s, boost::circular_buffer<uint8_t>::iterator& p1e, boost::circular_buffer<uint8_t>::iterator& p2s);
	void PacketProcessing_OutputSerialDataToConsole(const boost::circular_buffer<uint8_t>& serial_data, const boost::circular_buffer<uint8_t>::iterator& p1s, const boost::circular_buffer<uint8_t>::iterator& p1e, const boost::circular_buffer<uint8_t>::iterator& p2s);

}
// namespace AqualinkAutomate::Generators
