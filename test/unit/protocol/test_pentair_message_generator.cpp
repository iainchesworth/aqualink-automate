#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "pentair/generator/pentair_message_generator.h"
#include "pentair/messages/pentair_message_constants.h"
#include "pentair/messages/pentair_message_ids.h"
#include "pentair/messages/pentair_message_unknown.h"
#include "pentair/utility/pentair_checksum.h"

#include "errors/protocol_errors.h"

#include "kernel/data_hub.h"

#include "jandy/devices/aquarite_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/messages/jandy_message_ids.h"

#include "support/unit_test_mockreplayharness.h"
#include "support/unit_test_protocolmessagebuilder.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Pentair;

//=============================================================================
// Pentair B1: framing + parser + 16-bit checksum + protocol detection.
//
// These tests drive the FULL decode stack via the MockReplayHarness (which now
// registers both the Jandy and Pentair generators), feeding synthetic Pentair
// frames built by PentairMessageBuilder and asserting the decoded message that
// reaches the per-message static signal — plus the buffer-level behaviour of
// the generator (checksum rejection, incomplete frames, protocol coexistence).
//=============================================================================

namespace
{
	constexpr uint8_t CONTROLLER = 0x10;     // Master.
	constexpr uint8_t PUMP_60 = 0x60;        // First VSP pump address.

	// An arbitrary CMD byte that is NOT a known PentairMessageIds value, so it
	// decodes into PentairMessage_Unknown.
	constexpr uint8_t CMD_UNRECOGNISED = 0x77;
}

BOOST_AUTO_TEST_SUITE(TestSuite_PentairMessageGenerator)

//-----------------------------------------------------------------------------
// 16-bit checksum utility: big-endian sum over the checksummed region.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Checksum_SumsAllBytes)
{
	// 0xA5 + 0x10 + 0x60 + 0x07 + 0x02 + 0x01 + 0x02 = 0x121
	const std::vector<uint8_t> region = { 0xA5, 0x10, 0x60, 0x07, 0x02, 0x01, 0x02 };
	const uint16_t checksum = Pentair::Utility::PentairPacket_CalculateChecksum_FromRange(region);
	BOOST_CHECK_EQUAL(checksum, 0x0121u);
}

//-----------------------------------------------------------------------------
// Builder + generator round-trip: a well-formed frame parses into a message
// with the correct decoded header (FROM / DEST / CMD / LEN / payload).
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Replay_ValidFrame_DecodesHeaderAndPayload)
{
	Test::MockReplayHarness harness;

	// Capture the decoded message off the static signal.
	std::shared_ptr<const Pentair::Messages::PentairMessage_Unknown> captured;
	auto connection = Pentair::Messages::PentairMessage_Unknown::GetSignal()->connect(
		[&captured](const Pentair::Messages::PentairMessage_Unknown& msg)
		{
			captured = std::make_shared<Pentair::Messages::PentairMessage_Unknown>(msg);
		});

	const std::vector<uint8_t> payload = { 0xAA, 0xBB, 0xCC };
	auto frame = Test::PentairMessageBuilder::CreateValidChecksummedFrame(
		CONTROLLER, PUMP_60, CMD_UNRECOGNISED, payload);

	harness.Replay(frame);

	connection.disconnect();

	BOOST_TEST(harness.StatisticsHub()->MessageErrors.ChecksumFailures == 0u);
	BOOST_REQUIRE(captured != nullptr);
	BOOST_CHECK_EQUAL(captured->From(), CONTROLLER);
	BOOST_CHECK_EQUAL(captured->Destination(), PUMP_60);
	BOOST_CHECK_EQUAL(captured->RawCommand(), CMD_UNRECOGNISED);
	BOOST_CHECK_EQUAL(captured->DataLength(), static_cast<uint8_t>(payload.size()));
	BOOST_REQUIRE_EQUAL(captured->Payload().size(), payload.size());
	BOOST_CHECK_EQUAL(captured->Payload()[0], 0xAA);
	BOOST_CHECK_EQUAL(captured->Payload()[1], 0xBB);
	BOOST_CHECK_EQUAL(captured->Payload()[2], 0xCC);
}

//-----------------------------------------------------------------------------
// Direct generator call: a corrupt checksum is rejected, and ENOUGH of the bad
// frame is consumed to guarantee forward progress (the 4-byte preamble incl. the
// 0xA5 SOF is dropped so the same preamble is not re-matched), while leaving the
// trailing bytes to be re-offered.  A second parse then makes progress and never
// re-emits the corrupt frame.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Generator_CorruptChecksum_IsRejected)
{
	auto frame = Test::PentairMessageBuilder::CreateValidChecksummedFrame(
		CONTROLLER, PUMP_60, CMD_UNRECOGNISED, { 0x01, 0x02 });

	// Corrupt the low checksum byte (last byte of the frame).
	frame.back() ^= 0xFF;

	boost::circular_buffer<uint8_t> buffer(256);
	for (auto b : frame) { buffer.push_back(b); }

	const std::size_t size_before = buffer.size();
	auto result = Generators::GenerateMessageFromRawData(buffer);

	BOOST_REQUIRE(!result.has_value());
	BOOST_CHECK_EQUAL(result.error().value(), static_cast<int>(ErrorCodes::Protocol_ErrorCodes::ChecksumFailure));

	// The preamble (incl. the 0xA5 SOF) was consumed so the same corrupt frame is
	// not re-matched on the next pass; this guarantees forward progress.
	BOOST_CHECK_LT(buffer.size(), size_before);
	BOOST_CHECK_EQUAL(buffer.size(), size_before - 4u); // 0xFF 0x00 0xFF 0xA5 dropped.

	// A second parse finds no Pentair preamble in the residual bytes and defers
	// (WaitingForMoreData) without re-emitting any corrupt message.
	auto result2 = Generators::GenerateMessageFromRawData(buffer);
	BOOST_REQUIRE(!result2.has_value());
	BOOST_CHECK_EQUAL(result2.error().value(), static_cast<int>(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData));
}

//-----------------------------------------------------------------------------
// Direct generator call: a VALID complete Jandy frame followed by a CORRUPT
// Pentair-preamble frame must NOT lose the leading Jandy bytes.  On checksum
// failure the generator drops only up to/including the located 0xA5 SOF, so the
// leading bytes ahead of the preamble are re-offered for the Jandy generator to
// claim instead of being discarded.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Generator_LeadingBytesBeforeCorruptPreamble_AreReOffered)
{
	// Some leading (e.g. Jandy-looking) bytes ahead of the Pentair preamble.
	const std::vector<uint8_t> leading = { 0x10, 0x02, 0x50, 0x11, 0x28, 0x9B, 0x10, 0x03 };

	auto pentair = Test::PentairMessageBuilder::CreateValidChecksummedFrame(
		CONTROLLER, PUMP_60, CMD_UNRECOGNISED, { 0x01, 0x02 });
	pentair.back() ^= 0xFF; // Corrupt the Pentair checksum.

	boost::circular_buffer<uint8_t> buffer(256);
	for (auto b : leading) { buffer.push_back(b); }
	for (auto b : pentair) { buffer.push_back(b); }

	auto result = Generators::GenerateMessageFromRawData(buffer);

	BOOST_REQUIRE(!result.has_value());
	BOOST_CHECK_EQUAL(result.error().value(), static_cast<int>(ErrorCodes::Protocol_ErrorCodes::ChecksumFailure));

	// The leading bytes must survive (only the 4-byte Pentair preamble was
	// dropped), so they can be re-offered to another protocol's generator.
	BOOST_REQUIRE_GE(buffer.size(), leading.size());
	for (std::size_t i = 0; i < leading.size(); ++i)
	{
		BOOST_CHECK_EQUAL(buffer[i], leading[i]);
	}
}

//-----------------------------------------------------------------------------
// Direct generator call: an identified-but-incomplete frame (preamble located,
// DATA/checksum not yet arrived) DEFERS with DataAvailableToProcess — NOT
// WaitingForMoreData — and leaves the buffer untouched.
//
// CONTRACT: DataAvailableToProcess is the "my frame, still arriving" signal.
// The registry must NOT treat it as "try the next generator" (which is what
// WaitingForMoreData means); otherwise the destructive Jandy generator would
// clear() the buffer and destroy this in-flight frame.  See the consume-or-defer
// contract in MessageGeneratorRegistry::GenerateMessage.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Generator_IncompleteFrame_DefersWithoutConsuming)
{
	auto frame = Test::PentairMessageBuilder::CreateValidChecksummedFrame(
		CONTROLLER, PUMP_60, CMD_UNRECOGNISED, { 0x01, 0x02, 0x03, 0x04 });

	// Deliver only the preamble + header (no DATA / checksum yet).
	boost::circular_buffer<uint8_t> buffer(256);
	for (std::size_t i = 0; i < 3 + Pentair::Messages::HEADER_LENGTH; ++i) { buffer.push_back(frame[i]); }

	const std::size_t size_before = buffer.size();
	auto result = Generators::GenerateMessageFromRawData(buffer);

	BOOST_REQUIRE(!result.has_value());
	BOOST_CHECK_EQUAL(result.error().value(), static_cast<int>(ErrorCodes::Protocol_ErrorCodes::DataAvailableToProcess));
	BOOST_CHECK_EQUAL(buffer.size(), size_before); // untouched — the frame is preserved.
}

//-----------------------------------------------------------------------------
// Direct generator call: a preamble whose header has NOT fully arrived (fewer
// than HEADER_LENGTH bytes from the SOF) also defers with DataAvailableToProcess
// rather than discarding the partial preamble.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Generator_PreambleOnlyHeaderIncomplete_Defers)
{
	auto frame = Test::PentairMessageBuilder::CreateValidChecksummedFrame(
		CONTROLLER, PUMP_60, CMD_UNRECOGNISED, { 0x01 });

	// Deliver only the 4-byte preamble (0xFF 0x00 0xFF 0xA5) — no header fields.
	boost::circular_buffer<uint8_t> buffer(256);
	for (std::size_t i = 0; i < 4; ++i) { buffer.push_back(frame[i]); }

	const std::size_t size_before = buffer.size();
	auto result = Generators::GenerateMessageFromRawData(buffer);

	BOOST_REQUIRE(!result.has_value());
	BOOST_CHECK_EQUAL(result.error().value(), static_cast<int>(ErrorCodes::Protocol_ErrorCodes::DataAvailableToProcess));
	BOOST_CHECK_EQUAL(buffer.size(), size_before); // untouched.
}

//-----------------------------------------------------------------------------
// Protocol detection: a buffer with no Pentair preamble is left untouched so
// another protocol's generator can claim it.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Generator_NoPreamble_DefersWithoutConsuming)
{
	// A Jandy-looking frame (DLE/STX ...) — no Pentair preamble.
	const std::vector<uint8_t> jandy_like = { 0x10, 0x02, 0x50, 0x11, 0x28, 0x9B, 0x10, 0x03 };

	boost::circular_buffer<uint8_t> buffer(256);
	for (auto b : jandy_like) { buffer.push_back(b); }

	const std::size_t size_before = buffer.size();
	auto result = Generators::GenerateMessageFromRawData(buffer);

	BOOST_REQUIRE(!result.has_value());
	BOOST_CHECK_EQUAL(result.error().value(), static_cast<int>(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData));
	BOOST_CHECK_EQUAL(buffer.size(), size_before); // Jandy bytes preserved.
}

//-----------------------------------------------------------------------------
// Coexistence: with BOTH generators registered (as the harness does), replaying
// two valid Pentair frames back-to-back decodes both.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Replay_TwoFrames_BothDecode)
{
	Test::MockReplayHarness harness;

	int decode_count = 0;
	auto connection = Pentair::Messages::PentairMessage_Unknown::GetSignal()->connect(
		[&decode_count](const Pentair::Messages::PentairMessage_Unknown&) { ++decode_count; });

	auto frame1 = Test::PentairMessageBuilder::CreateValidChecksummedFrame(CONTROLLER, PUMP_60, CMD_UNRECOGNISED, { 0x01 });
	auto frame2 = Test::PentairMessageBuilder::CreateValidChecksummedFrame(PUMP_60, CONTROLLER, CMD_UNRECOGNISED, { 0x02, 0x03 });

	harness.Replay(std::vector<std::vector<uint8_t>>{ frame1, frame2 });

	connection.disconnect();

	BOOST_CHECK_EQUAL(decode_count, 2);
	BOOST_TEST(harness.StatisticsHub()->MessageErrors.ChecksumFailures == 0u);
}

//-----------------------------------------------------------------------------
// REGRESSION: a large (>128-byte, i.e. larger than one serial read chunk)
// Pentair frame delivered across TWO reads must NOT be destroyed between reads.
//
// Before the consume-or-defer fix, the partial first chunk (a located-but-
// incomplete Pentair frame) returned WaitingForMoreData; the registry then fell
// through to the destructive Jandy generator, which clear()ed the whole buffer.
// The second chunk then arrived as an orphaned tail and the frame was lost.
//
// With the fix the partial chunk defers (DataAvailableToProcess), the buffer is
// preserved, and the frame decodes once the remainder arrives.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Replay_LargeFrameSplitAcrossTwoReads_Decodes)
{
	Test::MockReplayHarness harness;

	std::shared_ptr<const Pentair::Messages::PentairMessage_Unknown> captured;
	int decode_count = 0;
	auto connection = Pentair::Messages::PentairMessage_Unknown::GetSignal()->connect(
		[&captured, &decode_count](const Pentair::Messages::PentairMessage_Unknown& msg)
		{
			captured = std::make_shared<Pentair::Messages::PentairMessage_Unknown>(msg);
			++decode_count;
		});

	// A payload large enough that the framed frame exceeds the 128-byte serial
	// read chunk, forcing the wire delivery to span more than one read.
	std::vector<uint8_t> payload(200);
	for (std::size_t i = 0; i < payload.size(); ++i) { payload[i] = static_cast<uint8_t>(i & 0xFF); }

	auto frame = Test::PentairMessageBuilder::CreateValidChecksummedFrame(
		CONTROLLER, PUMP_60, CMD_UNRECOGNISED, payload);
	BOOST_REQUIRE_GT(frame.size(), 128u);

	// Split the frame across two reads at the 128-byte boundary.
	const std::size_t split = 128;
	const std::vector<uint8_t> part1(frame.begin(), frame.begin() + static_cast<std::ptrdiff_t>(split));
	const std::vector<uint8_t> part2(frame.begin() + static_cast<std::ptrdiff_t>(split), frame.end());

	// First read: only the partial frame.  It must DEFER (no decode, no error)
	// and the bytes must be preserved in the protocol task's buffer.
	harness.Replay(part1);
	BOOST_CHECK_EQUAL(decode_count, 0);
	BOOST_TEST(harness.StatisticsHub()->MessageErrors.ChecksumFailures == 0u);
	BOOST_TEST(harness.StatisticsHub()->MessageErrors.BufferOverflows == 0u);

	// Second read: the remainder arrives; the full frame now decodes.
	harness.Replay(part2);

	connection.disconnect();

	BOOST_CHECK_EQUAL(decode_count, 1);
	BOOST_REQUIRE(captured != nullptr);
	BOOST_CHECK_EQUAL(captured->From(), CONTROLLER);
	BOOST_CHECK_EQUAL(captured->Destination(), PUMP_60);
	BOOST_CHECK_EQUAL(captured->DataLength(), static_cast<uint8_t>(payload.size()));
	BOOST_REQUIRE_EQUAL(captured->Payload().size(), payload.size());
	BOOST_CHECK_EQUAL(captured->Payload().front(), payload.front());
	BOOST_CHECK_EQUAL(captured->Payload().back(), payload.back());
	BOOST_TEST(harness.StatisticsHub()->MessageErrors.ChecksumFailures == 0u);
}

//-----------------------------------------------------------------------------
// REGRESSION: a VALID Jandy frame immediately followed by a CORRUPT
// Pentair-preamble frame.  The Pentair generator (priority 0) runs first; when
// its frame fails checksum it must drop ONLY its own preamble and re-offer the
// leading bytes, so the Jandy generator still decodes the leading Jandy frame
// into real device + DataHub state.
//
// Before the fix the checksum-failure path erased the leading region too,
// silently destroying the perfectly good Jandy frame ahead of the bad Pentair
// preamble.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Replay_ValidJandyThenCorruptPentair_JandyStillDecodes)
{
	using namespace AqualinkAutomate::Devices;

	Test::MockReplayHarness harness;

	// An AquaRite device so the leading Jandy frame decodes into observable state.
	constexpr uint8_t SWG_DEVICE_ID = 0x50;
	constexpr uint8_t MASTER_DEVICE_ID = 0x00;
	const uint8_t CMD_AQUARITE_PPM = static_cast<uint8_t>(AqualinkAutomate::Messages::JandyMessageIds::AQUARITE_PPM);
	constexpr uint8_t AQUARITE_STATUS_ON = 0x00;

	auto device_id = std::make_shared<JandyDeviceType>(JandyDeviceId(SWG_DEVICE_ID));
	(void)harness.AddDevice<AquariteDevice>(device_id);

	// A valid, checksummed Jandy AquaRite PPM frame (3200 PPM -> wire byte 0x20).
	constexpr uint16_t salt_ppm = 3200;
	constexpr uint8_t salt_ppm_wire = static_cast<uint8_t>(salt_ppm / 100);
	auto jandy = Test::MessageBuilder::CreateValidChecksummedMessage(
		MASTER_DEVICE_ID, CMD_AQUARITE_PPM, { salt_ppm_wire, AQUARITE_STATUS_ON });

	// A corrupt Pentair frame (valid preamble, broken checksum) right behind it.
	auto pentair = Test::PentairMessageBuilder::CreateValidChecksummedFrame(
		CONTROLLER, PUMP_60, CMD_UNRECOGNISED, { 0x03, 0x04 });
	pentair.back() ^= 0xFF;

	std::vector<uint8_t> combined;
	combined.insert(combined.end(), jandy.begin(), jandy.end());
	combined.insert(combined.end(), pentair.begin(), pentair.end());

	harness.Replay(combined);

	// Exactly one checksum failure (the Pentair frame) ...
	BOOST_TEST(harness.StatisticsHub()->MessageErrors.ChecksumFailures == 1u);

	// ... and the leading Jandy frame was re-offered and DECODED into the DataHub
	// rather than being destroyed alongside the bad Pentair preamble.
	BOOST_CHECK_EQUAL(harness.DataHub()->SaltLevel().value(), static_cast<double>(salt_ppm));
}

BOOST_AUTO_TEST_SUITE_END()
