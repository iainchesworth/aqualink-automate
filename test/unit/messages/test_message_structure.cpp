#include <cstdint>
#include <span>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/messages/jandy_message_ack.h"
#include "jandy/utility/jandy_checksum.h"
#include "jandy/utility/jandy_null_handler.h"

using namespace AqualinkAutomate::Messages;
using namespace AqualinkAutomate::Utility;

namespace
{
    // Build a valid, on-wire (escaped) ACK packet
    std::vector<uint8_t> make_valid_ack()
    {
        // Choose any valid destination; 0x10 is used in the examples, but not required.
        JandyMessage_Ack ack(AckTypes::ACK_IAQTouch, 0x10);
        std::vector<uint8_t> wire;
        BOOST_REQUIRE(ack.Serialize(wire)); // Ensure we start from a known-good packet
        return wire;
    }

    // Convenience: convert vector<uint8_t> to span<const std::byte> for Deserialize
    std::span<const std::byte> as_bytes_span(const std::vector<uint8_t>& v)
    {
        return std::as_bytes(std::span<const uint8_t>(v.data(), v.size()));
    }

    // Unescape on-wire to raw frame if you need to inspect/modify semantic fields
    std::vector<uint8_t> unescape(const std::vector<uint8_t>& wire)
    {
        std::vector<uint8_t> raw = wire;
        JandyPacket_NullCharHandler_Deserialization(raw);
        return raw;
    }

    // Escape raw to on-wire again (for when we mutate raw bytes and want a valid wire container)
    std::vector<uint8_t> escape(const std::vector<uint8_t>& raw)
    {
        std::vector<uint8_t> wire = raw;
        JandyPacket_NullCharHandler_Serialization(wire);
        return wire;
    }

    // Recompute checksum for a raw (unescaped) frame that already has header, payload,
    // and room for checksum + DLE,ETX at the tail. This helper matches the Serialize() logic:
    // checksum covers everything up to (but not including) the checksum byte itself.
    void recalc_checksum_in_raw(std::vector<uint8_t>& raw)
    {
        // checksum index is size-3: [.. payload ..][checksum][DLE][ETX]
        BOOST_REQUIRE(raw.size() >= 7); // DLE,STX,dest,id,chk,DLE,ETX minimal
        const std::size_t checksum_index = raw.size() - 3;
        const auto expected = JandyPacket_CalculateChecksum(raw.begin(), raw.begin() + checksum_index);
        raw[checksum_index] = expected;
    }
}

BOOST_AUTO_TEST_SUITE(JandyMessageStructure_TestSuite)

BOOST_AUTO_TEST_CASE(ValidAck_Deserialize_Succeeds)
{
    auto wire = make_valid_ack();
    JandyMessage_Ack ack(AckTypes::ACK_IAQTouch, 0x10);

    const bool ok = ack.Deserialize(as_bytes_span(wire));
    BOOST_CHECK(ok);
}

BOOST_AUTO_TEST_CASE(Size_TooShort_Fails)
{
    auto wire = make_valid_ack();

    // Force too-short: keep only 3 bytes (less than MINIMUM_PACKET_LENGTH)
    wire.resize(3);

    JandyMessage_Ack ack(AckTypes::ACK_IAQTouch, 0x10);
    const bool ok = ack.Deserialize(as_bytes_span(wire));
    BOOST_CHECK(!ok);
}

BOOST_AUTO_TEST_CASE(Size_TooLarge_Fails)
{
    auto wire = make_valid_ack();

    // Create a raw frame and inflate payload to exceed MAX length
    auto raw = unescape(wire);

    // Compute maximums from a temporary message object
    JandyMessage_Ack tmp(AckTypes::ACK_IAQTouch, 0x10);
    const auto max_len = tmp.MaxPermittedPacketLength();

    // Enlarge payload to exceed the maximum when re-escaped
    // Raw structure: [DLE,STX, dest, id, ...payload..., checksum, DLE, ETX]
    // Insert payload bytes before the checksum (at raw.size()-3)
    const std::size_t chk_index = raw.size() - 3;
    if (raw.size() <= max_len)
    {
        raw.insert(raw.begin() + chk_index, (max_len + 10) - raw.size(), 0x41);
    }

    recalc_checksum_in_raw(raw);
    wire = escape(raw);

    JandyMessage_Ack ack(AckTypes::ACK_IAQTouch, 0x10);
    const bool ok = ack.Deserialize(as_bytes_span(wire));
    BOOST_CHECK(!ok);
}

BOOST_AUTO_TEST_CASE(Framing_BadHeader_Fails)
{
    auto wire = make_valid_ack();
    auto raw = unescape(wire);

    // Corrupt header: first byte must be DLE
    raw[0] ^= 0xFF;

    wire = escape(raw);
    JandyMessage_Ack ack(AckTypes::ACK_IAQTouch, 0x10);
    const bool ok = ack.Deserialize(as_bytes_span(wire));
    BOOST_CHECK(!ok);
}

BOOST_AUTO_TEST_CASE(Framing_BadTrailer_Fails)
{
    auto wire = make_valid_ack();
    auto raw = unescape(wire);

    // Corrupt ETX (last byte)
    raw.back() ^= 0xFF;

    wire = escape(raw);
    JandyMessage_Ack ack(AckTypes::ACK_IAQTouch, 0x10);
    const bool ok = ack.Deserialize(as_bytes_span(wire));
    BOOST_CHECK(!ok);
}

BOOST_AUTO_TEST_CASE(Framing_Truncated_Missing_ETX_Fails)
{
    auto wire = make_valid_ack();
    auto raw = unescape(wire);

    // Remove ETX (last) and DLE (now last-1) => malformed tail
    if (raw.size() >= 2) { raw.pop_back(); }
    if (raw.size() >= 1) { raw.pop_back(); }

    // Do NOT escape; we're intentionally malformed
    wire = raw;
    JandyMessage_Ack ack(AckTypes::ACK_IAQTouch, 0x10);
    const bool ok = ack.Deserialize(as_bytes_span(wire));
    BOOST_CHECK(!ok);
}

BOOST_AUTO_TEST_CASE(Checksum_BadValue_Fails)
{
    auto wire = make_valid_ack();
    auto raw = unescape(wire);

    // Flip the checksum byte (size-3)
    BOOST_REQUIRE(raw.size() >= 7);
    const std::size_t checksum_index = raw.size() - 3;
    raw[checksum_index] ^= 0xFF;

    wire = escape(raw);
    JandyMessage_Ack ack(AckTypes::ACK_IAQTouch, 0x10);
    const bool ok = ack.Deserialize(as_bytes_span(wire));
    BOOST_CHECK(!ok);
}

BOOST_AUTO_TEST_CASE(Checksum_MissingByte_Fails)
{
    auto wire = make_valid_ack();
    auto raw = unescape(wire);

    // Remove the checksum byte (size-3), keep DLE,ETX -> should fail checksum/trailer logic
    BOOST_REQUIRE(raw.size() >= 7);
    raw.erase(raw.end() - 3);

    // Keep the (now) trailing DLE,ETX to mimic a malformed frame with no checksum
    wire = escape(raw);

    JandyMessage_Ack ack(AckTypes::ACK_IAQTouch, 0x10);
    const bool ok = ack.Deserialize(as_bytes_span(wire));
    BOOST_CHECK(!ok);
}

BOOST_AUTO_TEST_CASE(Checksum_Recalc_Then_Succeeds)
{
    auto wire = make_valid_ack();
    auto raw = unescape(wire);

    // Mutate payload (insert one byte) then fix checksum and check success
    const std::size_t checksum_index = raw.size() - 3;
    raw.insert(raw.begin() + checksum_index, 0x42); // add payload byte before checksum

    recalc_checksum_in_raw(raw);
    wire = escape(raw);

    JandyMessage_Ack ack(AckTypes::ACK_IAQTouch, 0x10);
    const bool ok = ack.Deserialize(as_bytes_span(wire));
    BOOST_CHECK(ok);
}

BOOST_AUTO_TEST_SUITE_END()