#pragma once

#include <cstdint>

namespace AqualinkAutomate::Utility
{

	template<typename MESSAGE_BYTES_SPAN_TYPE>
	uint8_t JandyPacket_CalculateChecksum(const MESSAGE_BYTES_SPAN_TYPE& message_bytes)
	{
		uint32_t checksum = 0;

		for (auto& elem : message_bytes)
		{
			checksum += static_cast<uint32_t>(elem);
		}

		return static_cast<uint8_t>(checksum & 0xFF);
	}

}
// namespace AqualinkAutomate::Utility
