#pragma once

#include <array>
#include <cstdint>

#include "jandy/messages/jandy_message_constants.h"

namespace AqualinkAutomate::Generators
{

	const std::array<uint8_t, 2> PACKET_START_SEQUENCE = { Messages::HEADER_BYTE_DLE, Messages::HEADER_BYTE_STX };
	const std::array<uint8_t, 2> PACKET_END_SEQUENCE = { Messages::HEADER_BYTE_DLE, Messages::HEADER_BYTE_ETX };

}
// namespace AqualinkAutomate::Generators
