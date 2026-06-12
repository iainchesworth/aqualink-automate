#pragma once

#include <cstdint>

#include <boost/circular_buffer.hpp>

namespace AqualinkAutomate::Generators
{

	struct PacketLocations
	{
		using Iterator = boost::circular_buffer<uint8_t>::iterator;

		PacketLocations() = default;
		PacketLocations(PacketLocations&& other) noexcept = default;
		PacketLocations& operator=(PacketLocations&& other) noexcept = default;
		PacketLocations(const PacketLocations&) = default;
		PacketLocations& operator=(const PacketLocations&) = default;

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
