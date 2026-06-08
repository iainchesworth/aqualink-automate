#include <cstddef>
#include <cstdint>
#include <deque>
#include <span>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "pentair/messages/pentair_message.h"
#include "pentair/messages/pentair_message_constants.h"
#include "pentair/messages/pentair_message_unknown.h"
#include "pentair/utility/pentair_checksum.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Pentair;

//=============================================================================
// PentairMessage base: contiguous-range concept, big-endian checksum helpers,
// checksum-ownership split between the hot (generator) path and the untrusted
// ISerializable byte-span path, and the self-defending size invariant.
//=============================================================================

namespace
{
	// Build a CHECKSUMMED REGION (0xA5 SOF .. CHK_LO) for the given header fields
	// and DATA payload, with LEN set from the payload and a correct trailing
	// 16-bit big-endian checksum.  This is the span the base Deserialize() consumes
	// (the generator strips the 0xFF/0x00/0xFF preamble before handing it over).
	std::vector<uint8_t> MakeChecksummedRegion(uint8_t from, uint8_t dest, uint8_t command, const std::vector<uint8_t>& payload)
	{
		std::vector<uint8_t> region;
		region.push_back(Messages::START_OF_FRAME);
		region.push_back(from);
		region.push_back(dest);
		region.push_back(command);
		region.push_back(static_cast<uint8_t>(payload.size())); // LEN
		region.insert(region.end(), payload.begin(), payload.end());

		const uint16_t checksum = Utility::PentairPacket_CalculateChecksum_FromRange(region);
		Utility::AppendBigEndianChecksum(region, checksum);
		return region;
	}

	std::vector<std::byte> ToByteSpanStorage(const std::vector<uint8_t>& bytes)
	{
		std::vector<std::byte> out(bytes.size());
		for (std::size_t i = 0; i < bytes.size(); ++i)
		{
			out[i] = static_cast<std::byte>(bytes[i]);
		}
		return out;
	}

	constexpr uint8_t CONTROLLER = 0x10;
	constexpr uint8_t PUMP_60 = 0x60;
	constexpr uint8_t CMD_UNRECOGNISED = 0x77;
}

BOOST_AUTO_TEST_SUITE(TestSuite_PentairMessageBase)

//-----------------------------------------------------------------------------
// The big-endian checksum helpers round-trip (CHK_HI first, CHK_LO second).
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(ChecksumHelpers_AppendAndReadRoundTrip)
{
	std::vector<uint8_t> buffer = { 0xA5, 0x10, 0x60 };
	Utility::AppendBigEndianChecksum(buffer, 0x1234u);

	BOOST_REQUIRE_EQUAL(buffer.size(), 5u);
	BOOST_CHECK_EQUAL(buffer[3], 0x12u); // high byte first
	BOOST_CHECK_EQUAL(buffer[4], 0x34u); // low byte second

	BOOST_CHECK_EQUAL(Utility::ReadBigEndianChecksum(std::span<const uint8_t>(buffer)), 0x1234u);
}

//-----------------------------------------------------------------------------
// The PentairRawMessageRange concept accepts contiguous sized uint8_t ranges
// (std::vector, std::span) and rejects non-contiguous ones (std::deque).
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Concept_AcceptsContiguousUint8Ranges)
{
	static_assert(Messages::PentairRawMessageRange<std::vector<uint8_t>>);
	static_assert(Messages::PentairRawMessageRange<std::span<const uint8_t>>);
	static_assert(!Messages::PentairRawMessageRange<std::deque<uint8_t>>);
	BOOST_CHECK(true);
}

//-----------------------------------------------------------------------------
// Hot path: the templated (contiguous-range) Deserialize decodes the header and
// payload of a well-formed frame.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(TemplatedDeserialize_ValidFrame_DecodesHeaderAndPayload)
{
	const std::vector<uint8_t> payload = { 0xAA, 0xBB, 0xCC };
	const auto region = MakeChecksummedRegion(CONTROLLER, PUMP_60, CMD_UNRECOGNISED, payload);

	Messages::PentairMessage_Unknown message;
	BOOST_REQUIRE(message.Deserialize(region));

	BOOST_CHECK_EQUAL(message.From(), CONTROLLER);
	BOOST_CHECK_EQUAL(message.Destination(), PUMP_60);
	BOOST_CHECK_EQUAL(message.RawCommand(), CMD_UNRECOGNISED);
	BOOST_CHECK_EQUAL(message.DataLength(), static_cast<uint8_t>(payload.size()));
	BOOST_REQUIRE_EQUAL(message.Payload().size(), payload.size());
	BOOST_CHECK_EQUAL(message.Payload()[0], 0xAAu);
	BOOST_CHECK_EQUAL(message.Payload()[2], 0xCCu);
}

//-----------------------------------------------------------------------------
// CHECKSUM OWNERSHIP (hot path): the templated Deserialize does NOT re-validate
// the checksum — the generator owns that validation and only hands over frames
// it has already verified.  A frame with a deliberately corrupt trailing
// checksum therefore still decodes via this path (it would never reach here on
// the real hot path with a bad checksum).  This proves the redundant second
// checksum recompute has been removed.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(TemplatedDeserialize_SkipsChecksumValidation)
{
	auto region = MakeChecksummedRegion(CONTROLLER, PUMP_60, CMD_UNRECOGNISED, { 0x01, 0x02 });
	region.back() ^= 0xFF; // Corrupt the low checksum byte.

	Messages::PentairMessage_Unknown message;
	BOOST_CHECK(message.Deserialize(region)); // Decodes despite the bad checksum.
}

//-----------------------------------------------------------------------------
// CHECKSUM OWNERSHIP (untrusted path): the ISerializable std::byte Deserialize
// overload is NOT pre-validated by the generator, so it verifies the checksum
// itself and REJECTS a corrupt frame.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(ByteSpanDeserialize_BadChecksum_IsRejected)
{
	auto region = MakeChecksummedRegion(CONTROLLER, PUMP_60, CMD_UNRECOGNISED, { 0x01, 0x02 });
	region.back() ^= 0xFF;

	const auto byte_storage = ToByteSpanStorage(region);

	Messages::PentairMessage_Unknown message;
	BOOST_CHECK(!message.Deserialize(std::span<const std::byte>(byte_storage)));
}

//-----------------------------------------------------------------------------
// The ISerializable std::byte overload accepts a well-formed, correctly
// checksummed frame and decodes it.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(ByteSpanDeserialize_ValidChecksum_Decodes)
{
	const auto region = MakeChecksummedRegion(CONTROLLER, PUMP_60, CMD_UNRECOGNISED, { 0x09 });
	const auto byte_storage = ToByteSpanStorage(region);

	Messages::PentairMessage_Unknown message;
	BOOST_REQUIRE(message.Deserialize(std::span<const std::byte>(byte_storage)));
	BOOST_CHECK_EQUAL(message.From(), CONTROLLER);
	BOOST_CHECK_EQUAL(message.DataLength(), 1u);
}

//-----------------------------------------------------------------------------
// SELF-DEFENDING SIZE INVARIANT: a frame whose advertised LEN is inconsistent
// with the actual region size is rejected by the size gate BEFORE any
// DeserializeContents read, so a subclass cannot over-run the span.  Here the
// LEN byte claims more DATA than the region actually contains.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Deserialize_OverstatedLength_IsRejected)
{
	// Build a valid 2-byte-payload region, then forge LEN to claim 200 bytes of
	// DATA that are not present.  FrameSizeIsValid must reject it.
	auto region = MakeChecksummedRegion(CONTROLLER, PUMP_60, CMD_UNRECOGNISED, { 0x01, 0x02 });
	region[Messages::Offset_Length] = 200;

	Messages::PentairMessage_Unknown message;
	BOOST_CHECK(!message.Deserialize(region)); // Hot path: size gate still runs.

	const auto byte_storage = ToByteSpanStorage(region);
	Messages::PentairMessage_Unknown message2;
	BOOST_CHECK(!message2.Deserialize(std::span<const std::byte>(byte_storage)));
}

//-----------------------------------------------------------------------------
// SELF-DEFENDING SIZE INVARIANT: a region shorter than the minimum frame length
// (header + checksum) is rejected without reading header fields out of bounds.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Deserialize_BelowMinimumLength_IsRejected)
{
	const std::vector<uint8_t> too_short = { Messages::START_OF_FRAME, 0x10, 0x60 };

	Messages::PentairMessage_Unknown message;
	BOOST_CHECK(!message.Deserialize(too_short));
}

BOOST_AUTO_TEST_SUITE_END()
