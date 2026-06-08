#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <boost/asio/error.hpp>

#include "kernel/statistics_hub.h"
#include "protocol/protocol_handler_constants.h"

#include "support/unit_test_mockreplayharness.h"

using namespace AqualinkAutomate;

//=============================================================================
// Capture-replay per-frame read bound (ProtocolTask).
//
// --replay-filename feeds a capture file through a MockSerialPortImpl whose
// read_some hands back the whole file as fast as the parser asks for it.  With
// no bound the ProtocolTask drains the ENTIRE capture in a single Poll() — that
// overruns the fixed-size circular buffer (only the tail survives, so frames in
// the middle of a long capture are silently lost) and there is nothing left for
// the application's frame loop to pace.
//
// Pacing is owned by the application frame loop (it steps at a fixed processing
// period; see src/aqualink-automate.cpp); the ProtocolTask's part is to read at
// most ONE serial chunk per Poll() so each frame advances the capture by a
// bounded amount.  These tests drive the REAL ProtocolTask via MockReplayHarness
// and assert that per-frame read bound and the no-overflow guarantee it gives —
// both deterministic.  The harness defaults to the unbounded (drain-per-poll)
// behaviour, so the rest of the suite is unaffected; the per-frame sleep itself
// lives in the main loop and is covered by manual capture replay, not here.
//=============================================================================

namespace
{
	// A filler byte that is NOT a Jandy (DLE 0x10) or Pentair (0xFF) packet
	// start, so the generator finds no frame and clears the buffer each cycle —
	// keeping the overflow behaviour purely a function of how much DrainReads
	// reads per Poll(), not of parser back-pressure.
	constexpr std::uint8_t FILLER = 0xAA;

	// Enough full-size chunks to exceed the circular buffer if slurped at once.
	constexpr std::size_t CHUNKS_TO_OVERFLOW =
		(Protocol::Constants::SERIAL_CIRCULAR_BUFFER_SIZE / Protocol::Constants::SERIAL_READ_CHUNK_SIZE) + 4;
}

BOOST_AUTO_TEST_SUITE(TestSuite_ReplayPacing)

//-----------------------------------------------------------------------------
// Per-frame read bound: a single-read-per-poll Poll() consumes exactly one read.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(PacedReplay_ReadsAtMostOneChunkPerPoll)
{
	Test::MockReplayHarness harness(/*single_read_per_poll=*/true);
	auto& serial = harness.SerialImpl();

	// Five chunks are available right now plus a would_block sentinel that an
	// unbounded drain would run all the way to.
	for (int i = 0; i < 5; ++i)
	{
		serial.QueueReadData(std::vector<std::uint8_t>(64, FILLER));
	}
	serial.QueueReadData({}, boost::asio::error::would_block);

	const auto reads_before = serial.GetReadCallCount();
	(void)harness.ProtocolTask().Poll();
	const auto reads_after = serial.GetReadCallCount();

	// Exactly one read_some this frame — the remaining chunks wait for later polls.
	BOOST_CHECK_EQUAL(reads_after - reads_before, 1u);
	BOOST_TEST(harness.StatisticsHub()->MessageErrors.BufferOverflows == 0u);
}

//-----------------------------------------------------------------------------
// Default (unbounded) behaviour is unchanged: one Poll() drains everything that
// is available, stopping only at the would_block sentinel.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(UnpacedReplay_DrainsAllAvailableInOnePoll)
{
	Test::MockReplayHarness harness;   // default = drain per poll
	auto& serial = harness.SerialImpl();

	for (int i = 0; i < 5; ++i)
	{
		serial.QueueReadData(std::vector<std::uint8_t>(64, FILLER));
	}
	serial.QueueReadData({}, boost::asio::error::would_block);

	const auto reads_before = serial.GetReadCallCount();
	(void)harness.ProtocolTask().Poll();
	const auto reads_after = serial.GetReadCallCount();

	// Five data reads + the would_block read that ends the drain loop.
	BOOST_CHECK_EQUAL(reads_after - reads_before, 6u);
}

//-----------------------------------------------------------------------------
// No overflow under high volume: the per-frame read bound keeps the circular
// buffer bounded even when far more than a buffer's worth of bytes is waiting.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(PacedReplay_DoesNotOverflowCircularBufferUnderHighVolume)
{
	Test::MockReplayHarness harness(/*single_read_per_poll=*/true);
	auto& serial = harness.SerialImpl();

	for (std::size_t i = 0; i < CHUNKS_TO_OVERFLOW; ++i)
	{
		serial.QueueReadData(std::vector<std::uint8_t>(Protocol::Constants::SERIAL_READ_CHUNK_SIZE, FILLER));
	}
	serial.QueueReadData({}, boost::asio::error::would_block);

	// Drain one chunk per Poll() until everything queued has been consumed.
	std::size_t polls = 0;
	const std::size_t poll_guard = CHUNKS_TO_OVERFLOW + 8;
	while (!serial.IsReadQueueEmpty() && (polls < poll_guard))
	{
		(void)harness.ProtocolTask().Poll();
		++polls;
	}

	BOOST_CHECK(serial.IsReadQueueEmpty());
	BOOST_TEST(harness.StatisticsHub()->MessageErrors.BufferOverflows == 0u);
}

//-----------------------------------------------------------------------------
// The bug being fixed, pinned: the unbounded path slurps the same high volume in
// a single Poll() and overruns the circular buffer.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(UnpacedReplay_SlurpsAndOverflowsCircularBuffer)
{
	Test::MockReplayHarness harness;   // default = drain per poll
	auto& serial = harness.SerialImpl();

	for (std::size_t i = 0; i < CHUNKS_TO_OVERFLOW; ++i)
	{
		serial.QueueReadData(std::vector<std::uint8_t>(Protocol::Constants::SERIAL_READ_CHUNK_SIZE, FILLER));
	}
	serial.QueueReadData({}, boost::asio::error::would_block);

	(void)harness.ProtocolTask().Poll();

	// One Poll() read past the buffer's capacity, discarding the surplus.
	BOOST_TEST(harness.StatisticsHub()->MessageErrors.BufferOverflows > 0u);
}

BOOST_AUTO_TEST_SUITE_END()
