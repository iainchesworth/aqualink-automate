#pragma once

#include <cstdint>

namespace AqualinkAutomate::Messages
{

	constexpr uint8_t HEADER_BYTE_DLE = 0x10;
	constexpr uint8_t HEADER_BYTE_STX = 0x02;
	constexpr uint8_t HEADER_BYTE_ETX = 0x03;

}
// namespace AqualinkAutomate::Messages
