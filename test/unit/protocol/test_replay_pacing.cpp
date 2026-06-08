#include <chrono>
#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <boost/asio/error.hpp>

#include "kernel/statistics_hub.h"
#include "protocol/protocol_handler_constants.h"

#include "support/unit_test_mockreplayharness.h"

using namespace AqualinkAutomate;

//=============================================================================
// Capture-replay pacing (ProtocolTask).
//
// --replay-filename feeds a capture file through a MockSerialPortImpl whose
// read_some hands back the whole file as fast as the parser asks for it.  With
// no pacing the ProtocolTask drains the ENTIRE capture in a single Poll() — that
// both defeats any notion of bus-rate delivery AND overruns the fixed-size
// circular buffer (only the tail survives, so frames in the middle of a long
// capture are silently lost).
//
// Pacing fixes both: when a non-zero replay_frame_period is configured the poll
// loop reads at most ONE serial chunk per Poll() and then sleeps the remainder
// of the period, so successive cycles are spaced at roughly the bus's natural
// inter-frame rate and the buffer never accumulates more than a chunk.
//
// These tests drive the REAL ProtocolTask via the MockReplayHarness, asserting
// the per-cycle read bound (deterministic), the no-overflow guarantee under
// high volume (deterministic) and that the per-cycle sleep actually happens
// (a robust lower-bound timing check).  The harness defaults to UNPACED, so the
// rest of the suite is unaffected.
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
// Per-cycle read bound: a paced Poll() consumes exactly one serial read.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(PacedReplay_ReadsAtMostOneChunkPerPoll)
{
	using namespace std::chrono_literals;
	Test::MockReplayHarness harness(10ms);   // paced
	auto& serial = harness.SerialImpl();

	// Five chunks are available right now plus a would_block sentinel that an
	// UNPACED drain would run all the way to.
	for (int i = 0; i < 5; ++i)
	{
		serial.QueueReadData(std::vector<std::uint8_t>(64, FILLER));
	}
	serial.QueueReadData({}, boost::asio::error::would_block);

	const auto reads_before = serial.GetReadCallCount();
	(void)harness.ProtocolTask().Poll();
	const auto reads_after = serial.GetReadCallCount();

	// Exactly one read_some this cycle — the remaining chunks wait for later polls.
	BOOST_CHECK_EQUAL(reads_after - reads_before, 1u);
	BOOST_TEST(harness.StatisticsHub()->MessageErrors.BufferOverflows == 0u);
}

//-----------------------------------------------------------------------------
// Default (unpaced) behaviour is unchanged: one Poll() drains everything that is
// available, stopping only at the would_block sentinel.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(UnpacedReplay_DrainsAllAvailableInOnePoll)
{
	Test::MockReplayHarness harness;   // default = unpaced (period 0)
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
// No overflow under high volume: paced reads keep the circular buffer bounded
// even when far more than a buffer's worth of bytes is waiting.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(PacedReplay_DoesNotOverflowCircularBufferUnderHighVolume)
{
	// A 1us period keeps the bounded-read path active while making the per-cycle
	// sleep effectively free (the deadline has already passed by the time the
	// read+parse work finishes), so the drain loop below stays fast.
	Test::MockReplayHarness harness(std::chrono::microseconds(1));   // paced
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
// The bug being fixed, pinned: the UNPACED path slurps the same high volume in a
// single Poll() and overruns the circular buffer.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(UnpacedReplay_SlurpsAndOverflowsCircularBuffer)
{
	Test::MockReplayHarness harness;   // default = unpaced (period 0)
	auto& serial = harness.SerialImpl();

	for (std::size_t i = 0; i < CHUNKS_TO_OVERFLOW; ++i)
	{
		serial.QueueReadData(std::vector<std::uint8_t>(Protocol::Constants::SERIAL_READ_CHUNK_SIZE, FILLER));
	}
	serial.QueueReadData({}, boost::asio::error::would_block);

	(void)harness.ProtocolTask().Poll();

	// One cycle read past the buffer's capacity, discarding the surplus.
	BOOST_TEST(harness.StatisticsHub()->MessageErrors.BufferOverflows > 0u);
}

//-----------------------------------------------------------------------------
// The pacing sleep actually happens: a paced Poll() takes at least (most of) one
// period.  Lower-bound only — sleep_until never returns early — so this stays
// robust under load.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(PacedReplay_SleepsAtLeastOnePeriodPerPoll)
{
	using namespace std::chrono;
	constexpr auto period = milliseconds(25);
	Test::MockReplayHarness harness(period);   // paced
	auto& serial = harness.SerialImpl();

	// No data to parse — just a would_block so the cycle does its read, finds
	// nothing, and proceeds to the pacing sleep.
	serial.QueueReadData({}, boost::asio::error::would_block);

	const auto start = steady_clock::now();
	(void)harness.ProtocolTask().Poll();
	const auto elapsed = steady_clock::now() - start;

	// Conservative lower bound (period minus slack) to avoid coupling to clock
	// granularity while still proving the cycle slept.
	BOOST_CHECK_GE(duration_cast<milliseconds>(elapsed).count(), 20);
}

BOOST_AUTO_TEST_SUITE_END()
