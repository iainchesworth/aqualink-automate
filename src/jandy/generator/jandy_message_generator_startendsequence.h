#pragma once

#include <array>
#include <cstdint>

#include "messages/jandy_message_constants.h"

namespace AqualinkAutomate::Generators
{

	inline constexpr std::array<uint8_t, 2> PACKET_START_SEQUENCE = { Messages::HEADER_BYTE_DLE, Messages::HEADER_BYTE_STX };
	inline constexpr std::array<uint8_t, 2> PACKET_END_SEQUENCE = { Messages::HEADER_BYTE_DLE, Messages::HEADER_BYTE_ETX };

}
// namespace AqualinkAutomate::Generators
