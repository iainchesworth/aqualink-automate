#pragma once

#include <cstdint>
#include <vector>

namespace AqualinkAutomate::Generators
{

	void BufferCleanUp_ClearBytesFromBeginToPos(std::vector<uint8_t>& serial_data, const std::vector<uint8_t>::iterator& position);
	void BufferCleanUp_HasEndOfPacketWithinMaxDistance(std::vector<uint8_t>& serial_data, const std::vector<uint8_t>::iterator& p1s, const std::vector<uint8_t>::iterator& p1e, const std::vector<uint8_t>::iterator& p2s);

}
// namespace AqualinkAutomate::Generators
