#pragma once

#include <cstdint>
#include <span>

#include <boost/circular_buffer.hpp>

namespace AqualinkAutomate::Generators
{

	struct PacketLocations
	{
		using Iterator = boost::circular_buffer<uint8_t>::iterator;

		Iterator p1_start;
		Iterator p1_end;
		Iterator p2_start;

		bool HasPacketStart{false};
		bool HasPacketEnd{false};
		bool HasSecondPacketStart{false};
	};

	PacketLocations PacketProcessing_FindAllPacketLocations(boost::circular_buffer<uint8_t>& serial_data);
	void PacketProcessing_OutputSerialDataToConsole(const boost::circular_buffer<uint8_t>& serial_data, const PacketLocations& locations);

}
// namespace AqualinkAutomate::Generators
