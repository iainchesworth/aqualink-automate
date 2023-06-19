#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace AqualinkAutomate::Generators
{

	void PacketProcessing_GetPacketLocations(std::vector<uint8_t>& serial_data, std::vector<uint8_t>::iterator& p1s, std::vector<uint8_t>::iterator& p1e, std::vector<uint8_t>::iterator& p2s);
	void PacketProcessing_OutputSerialDataToConsole(const std::vector<uint8_t>& serial_data, const std::vector<uint8_t>::iterator& p1s, const std::vector<uint8_t>::iterator& p1e, const std::vector<uint8_t>::iterator& p2s);

}
// namespace AqualinkAutomate::Generators
