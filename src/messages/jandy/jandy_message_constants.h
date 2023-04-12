#pragma once

#include <cstdint>

namespace AqualinkAutomate::Messages::Jandy
{

	constexpr uint8_t MAXIMUM_PACKET_LENGTH = 128;

	constexpr uint8_t HEADER_BYTE_DLE = 0x10;
	constexpr uint8_t HEADER_BYTE_STX = 0x02;
	constexpr uint8_t HEADER_BYTE_ETX = 0x03;

	enum class JandyMessageTypes : uint8_t
	{
		Probe			= 0x00,
		Ack				= 0x01,
		Status			= 0x02,
		Message			= 0x03,
		MessageLong		= 0x04,
		MessageLoopSt	= 0x05,
		Unknown
	};

}
// namespace AqualinkAutomate::Messages::Jandy
