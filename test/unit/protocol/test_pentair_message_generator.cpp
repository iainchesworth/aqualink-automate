#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "pentair/generator/pentair_message_generator.h"
#include "pentair/messages/pentair_message_constants.h"
#include "pentair/messages/pentair_message_ids.h"
#include "pentair/messages/pentair_message_unknown.h"
#include "pentair/utility/pentair_checksum.h"

#include "errors/protocol_errors.h"

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
	std::shared_ptr<const Messages::PentairMessage_Unknown> captured;
	auto connection = Messages::PentairMessage_Unknown::GetSignal()->connect(
		[&captured](const Messages::PentairMessage_Unknown& msg)
		{
			captured = std::make_shared<Messages::PentairMessage_Unknown>(msg);
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
// Direct generator call: a corrupt checksum is rejected, the bad frame is
// consumed (so the stream makes progress), and no message is produced.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Generator_CorruptChecksum_IsRejected)
{
	auto frame = Test::PentairMessageBuilder::CreateValidChecksummedFrame(
		CONTROLLER, PUMP_60, CMD_UNRECOGNISED, { 0x01, 0x02 });

	// Corrupt the low checksum byte (last byte of the frame).
	frame.back() ^= 0xFF;

	boost::circular_buffer<uint8_t> buffer(256);
	for (auto b : frame) { buffer.push_back(b); }

	auto result = Generators::GenerateMessageFromRawData(buffer);

	BOOST_REQUIRE(!result.has_value());
	BOOST_CHECK_EQUAL(result.error().value(), static_cast<int>(ErrorCodes::Protocol_ErrorCodes::ChecksumFailure));
	// The bad frame was consumed so the next parse can proceed.
	BOOST_CHECK(buffer.empty());
}

//-----------------------------------------------------------------------------
// Direct generator call: an incomplete frame (header arrived, data not yet)
// returns WaitingForMoreData and leaves the buffer untouched.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Generator_IncompleteFrame_WaitsForMoreData)
{
	auto frame = Test::PentairMessageBuilder::CreateValidChecksummedFrame(
		CONTROLLER, PUMP_60, CMD_UNRECOGNISED, { 0x01, 0x02, 0x03, 0x04 });

	// Deliver only the preamble + header (no DATA / checksum yet).
	boost::circular_buffer<uint8_t> buffer(256);
	for (std::size_t i = 0; i < 3 + Messages::HEADER_LENGTH; ++i) { buffer.push_back(frame[i]); }

	const std::size_t size_before = buffer.size();
	auto result = Generators::GenerateMessageFromRawData(buffer);

	BOOST_REQUIRE(!result.has_value());
	BOOST_CHECK_EQUAL(result.error().value(), static_cast<int>(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData));
	BOOST_CHECK_EQUAL(buffer.size(), size_before); // untouched
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
	auto connection = Messages::PentairMessage_Unknown::GetSignal()->connect(
		[&decode_count](const Messages::PentairMessage_Unknown&) { ++decode_count; });

	auto frame1 = Test::PentairMessageBuilder::CreateValidChecksummedFrame(CONTROLLER, PUMP_60, CMD_UNRECOGNISED, { 0x01 });
	auto frame2 = Test::PentairMessageBuilder::CreateValidChecksummedFrame(PUMP_60, CONTROLLER, CMD_UNRECOGNISED, { 0x02, 0x03 });

	harness.Replay(std::vector<std::vector<uint8_t>>{ frame1, frame2 });

	connection.disconnect();

	BOOST_CHECK_EQUAL(decode_count, 2);
	BOOST_TEST(harness.StatisticsHub()->MessageErrors.ChecksumFailures == 0u);
}

BOOST_AUTO_TEST_SUITE_END()
