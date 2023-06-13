#pragma once

#include <cstdint>
#include <vector>

namespace AqualinkAutomate::Utility
{

	void JandyPacket_NullCharHandler_Serialization(std::vector<uint8_t>& message_bytes);
	void JandyPacket_NullCharHandler_Deserialization(std::vector<uint8_t>& message_bytes);

}
// namespace AqualinkAutomate::Utility
