#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/messages/jandy_message.h"
#include "jandy/messages/jandy_message_constants.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_unknown.h"
#include "jandy/utility/jandy_checksum.h"

using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Messages;

namespace
{
	// Build a minimally-framed, correctly-checksummed Jandy frame:
	//   DLE STX dest cmd [payload...] checksum DLE ETX
	// The checksum is the 8-bit sum of every byte before it (indices [0, size-3)),
	// matching JandyMessage::Serialize / PacketChecksumIsValid.
	std::vector<uint8_t> MakeFramedPacket(uint8_t destination, uint8_t command, const std::vector<uint8_t>& payload)
	{
		std::vector<uint8_t> frame;
		frame.reserve(payload.size() + 7);

		frame.push_back(HEADER_BYTE_DLE);
		frame.push_back(HEADER_BYTE_STX);
		frame.push_back(destination);
		frame.push_back(command);
		frame.insert(frame.end(), payload.begin(), payload.end());

		const uint8_t checksum = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(frame.begin(), frame.end());
		frame.push_back(checksum);
		frame.push_back(HEADER_BYTE_DLE);
		frame.push_back(HEADER_BYTE_ETX);

		return frame;
	}
}

BOOST_AUTO_TEST_SUITE(JandyMessageTestSuite)

//=============================================================================
// WU-JANDY-MESSAGE-BASE regression coverage.
//
// JandyMessage exposes two Deserialize entry points that funnel into one shared
// contiguous-data implementation:
//   * Deserialize(std::span<const std::byte>&) -- the ISerializable cold path,
//     which copies into a stack buffer sized by MAXIMUM_PACKET_LENGTH.
//   * Deserialize(const JandyRawMessageRange&)  -- the contiguous uint8_t hot
//     path used by the message factory.
// These tests prove both decode an identical valid frame to identical state and
// that the std::byte path's stack buffer correctly handles a packet at the
// MAXIMUM_PACKET_LENGTH upper bound (the buffer-coupling fix).
//=============================================================================

// The templated uint8_t-range overload (hot path) decodes a valid frame.
BOOST_AUTO_TEST_CASE(Deserialize_Uint8Range_DecodesValidFrame)
{
	const std::vector<uint8_t> frame = MakeFramedPacket(0x00, 0x29, { 0x11, 0x1c });

	JandyMessage_Unknown message;
	BOOST_REQUIRE(message.Deserialize(std::span<const uint8_t>(frame)));

	BOOST_CHECK_EQUAL(message.Destination().Id()(), 0x00);
	BOOST_CHECK_EQUAL(message.RawId(), 0x29);
	BOOST_REQUIRE_EQUAL(message.Payload().size(), 2u);
	BOOST_CHECK_EQUAL(message.Payload()[0], 0x11);
	BOOST_CHECK_EQUAL(message.Payload()[1], 0x1c);
}

// The std::byte ISerializable overload decodes the same frame to the same state.
BOOST_AUTO_TEST_CASE(Deserialize_ByteSpan_DecodesValidFrame)
{
	const std::vector<uint8_t> frame = MakeFramedPacket(0x00, 0x29, { 0x11, 0x1c });

	JandyMessage_Unknown message;
	BOOST_REQUIRE(message.Deserialize(std::as_bytes(std::span<const uint8_t>(frame))));

	BOOST_CHECK_EQUAL(message.Destination().Id()(), 0x00);
	BOOST_CHECK_EQUAL(message.RawId(), 0x29);
	BOOST_REQUIRE_EQUAL(message.Payload().size(), 2u);
	BOOST_CHECK_EQUAL(message.Payload()[0], 0x11);
	BOOST_CHECK_EQUAL(message.Payload()[1], 0x1c);
}

// Both overloads must agree byte-for-byte on the decoded state.
BOOST_AUTO_TEST_CASE(Deserialize_BothOverloads_ProduceIdenticalState)
{
	const std::vector<uint8_t> frame = MakeFramedPacket(0x50, 0x16, { 0x01, 0x02, 0x03 });

	JandyMessage_Unknown via_range;
	JandyMessage_Unknown via_bytes;

	BOOST_REQUIRE(via_range.Deserialize(std::span<const uint8_t>(frame)));
	BOOST_REQUIRE(via_bytes.Deserialize(std::as_bytes(std::span<const uint8_t>(frame))));

	BOOST_CHECK_EQUAL(via_range.Destination().Id()(), via_bytes.Destination().Id()());
	BOOST_CHECK_EQUAL(via_range.RawId(), via_bytes.RawId());
	BOOST_CHECK_EQUAL(via_range.MessageLength(), via_bytes.MessageLength());
	BOOST_CHECK(via_range.Payload() == via_bytes.Payload());
}

// Buffer-coupling fix: a frame sized exactly at MAXIMUM_PACKET_LENGTH must be
// accepted by the std::byte overload (its stack buffer is now sized by
// MAXIMUM_PACKET_LENGTH, not a bare 128 literal that could drift apart from the
// PacketSizeIsValid upper bound).  A larger frame must be rejected by both
// overloads without overrunning that buffer.
BOOST_AUTO_TEST_CASE(Deserialize_ByteSpan_FrameAtMaximumLength_IsAccepted)
{
	// Header (4) + payload + footer (3) == MAXIMUM_PACKET_LENGTH.
	constexpr std::size_t framing_overhead = static_cast<std::size_t>(JandyMessage::PACKET_HEADER_LENGTH) + JandyMessage::PACKET_FOOTER_LENGTH;
	const std::size_t payload_size = static_cast<std::size_t>(JandyMessage::MAXIMUM_PACKET_LENGTH) - framing_overhead;

	const std::vector<uint8_t> payload(payload_size, 0xAB);
	const std::vector<uint8_t> frame = MakeFramedPacket(0x00, 0x29, payload);
	BOOST_REQUIRE_EQUAL(frame.size(), static_cast<std::size_t>(JandyMessage::MAXIMUM_PACKET_LENGTH));

	JandyMessage_Unknown via_bytes;
	BOOST_CHECK(via_bytes.Deserialize(std::as_bytes(std::span<const uint8_t>(frame))));

	JandyMessage_Unknown via_range;
	BOOST_CHECK(via_range.Deserialize(std::span<const uint8_t>(frame)));
}

// A frame larger than MAXIMUM_PACKET_LENGTH is rejected (size guard fires before
// the stack-buffer copy in the std::byte path, so the buffer is never overrun).
BOOST_AUTO_TEST_CASE(Deserialize_ByteSpan_OversizeFrame_IsRejected)
{
	const std::vector<uint8_t> oversize(static_cast<std::size_t>(JandyMessage::MAXIMUM_PACKET_LENGTH) + 8, 0xCD);

	JandyMessage_Unknown via_bytes;
	BOOST_CHECK(!via_bytes.Deserialize(std::as_bytes(std::span<const uint8_t>(oversize))));
}

BOOST_AUTO_TEST_SUITE_END()
