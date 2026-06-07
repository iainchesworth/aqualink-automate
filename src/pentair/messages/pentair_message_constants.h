#pragma once

#include <array>
#include <cstdint>

namespace AqualinkAutomate::Pentair::Messages
{

	//-------------------------------------------------------------------------
	// PENTAIR RS-485 FRAMING
	//
	// Wire layout:
	//   [0xFF][0x00][0xFF][0xA5][FROM][DEST][CMD][LEN][DATA...][CHK_HI][CHK_LO]
	//    \-------preamble------/ \---------- header --------/
	//
	// The frame is preceded by an idle/preamble sequence ending in 0xFF, then a
	// 0xA5 start-of-frame byte.  A 16-bit big-endian checksum (the running sum of
	// every byte from the 0xA5 SOF through the final DATA byte) terminates it.
	//
	// Protocol auto-detection keys off the leading 0xFF preamble byte, which the
	// Jandy DLE/STX framing never produces at a packet boundary.
	//-------------------------------------------------------------------------

	constexpr uint8_t PREAMBLE_BYTE_FF = 0xFF;
	constexpr uint8_t PREAMBLE_BYTE_00 = 0x00;
	constexpr uint8_t START_OF_FRAME   = 0xA5;

	// The canonical 4-byte preamble that immediately precedes the 0xA5 SOF.
	inline constexpr std::array<uint8_t, 4> FRAME_PREAMBLE = {
		PREAMBLE_BYTE_FF, PREAMBLE_BYTE_00, PREAMBLE_BYTE_FF, START_OF_FRAME
	};

	//-------------------------------------------------------------------------
	// FIELD OFFSETS (relative to the 0xA5 SOF byte, i.e. into the checksummed
	// region).  Offset 0 is the 0xA5 itself.
	//-------------------------------------------------------------------------

	constexpr uint8_t Offset_StartOfFrame = 0;
	constexpr uint8_t Offset_From         = 1;
	constexpr uint8_t Offset_Dest         = 2;
	constexpr uint8_t Offset_Command      = 3;
	constexpr uint8_t Offset_Length       = 4;
	constexpr uint8_t Offset_DataStart    = 5;

	// Bytes from the 0xA5 SOF up to (and including) the LEN field, i.e. the
	// fixed header that always precedes the variable-length DATA section.
	constexpr uint8_t HEADER_LENGTH = 5;

	// Two trailing checksum bytes (big-endian: high then low).
	constexpr uint8_t CHECKSUM_LENGTH = 2;

	// Smallest legal checksummed region: header + zero data + checksum.
	constexpr uint8_t MINIMUM_FRAME_LENGTH = HEADER_LENGTH + CHECKSUM_LENGTH;

	// Defensive upper bound on a single frame's checksummed region.  LEN is a
	// single byte, so DATA cannot exceed 255 bytes.
	constexpr uint16_t MAXIMUM_FRAME_LENGTH = HEADER_LENGTH + 255 + CHECKSUM_LENGTH;

	//-------------------------------------------------------------------------
	// DEVICE ADDRESSES
	//-------------------------------------------------------------------------

	constexpr uint8_t CONTROLLER_ADDRESS = 0x10; // Controller / master (IntelliCenter, EasyTouch).
	constexpr uint8_t PUMP_ADDRESS_BASE  = 0x60; // VSP pumps occupy 0x60 - 0x6F.
	constexpr uint8_t PUMP_ADDRESS_LAST  = 0x6F;

}
// namespace AqualinkAutomate::Pentair::Messages
