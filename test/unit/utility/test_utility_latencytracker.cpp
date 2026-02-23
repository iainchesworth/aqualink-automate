#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "utility/latency_percentile_tracker.h"

using namespace AqualinkAutomate::Utility;
using namespace std::chrono_literals;

BOOST_AUTO_TEST_SUITE(LatencyPercentileTracker_TestSuite)

//-----------------------------------------------------------------------------
// BASIC FUNCTIONALITY
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_EmptyTracker_ReturnsZeroPercentiles)
{
	LatencyPercentileTracker<> tracker;
	auto snapshot = tracker.GetSnapshot();

	BOOST_CHECK_EQUAL(snapshot.sample_count, 0);
	BOOST_CHECK_EQUAL(snapshot.p1.count(), 0);
	BOOST_CHECK_EQUAL(snapshot.p50.count(), 0);
	BOOST_CHECK_EQUAL(snapshot.p99.count(), 0);
	BOOST_CHECK_EQUAL(snapshot.min.count(), 0);
	BOOST_CHECK_EQUAL(snapshot.max.count(), 0);
	BOOST_CHECK_EQUAL(snapshot.alltime_min.count(), 0);
	BOOST_CHECK_EQUAL(snapshot.alltime_max.count(), 0);
	BOOST_CHECK_EQUAL(snapshot.mean.count(), 0);
}

BOOST_AUTO_TEST_CASE(Test_SingleSample_AllPercentilesEqual)
{
	LatencyPercentileTracker<> tracker;
	tracker.Record(1000ns);

	auto snapshot = tracker.GetSnapshot();

	BOOST_CHECK_EQUAL(snapshot.sample_count, 1);
	BOOST_CHECK_EQUAL(snapshot.p1.count(), 1000);
	BOOST_CHECK_EQUAL(snapshot.p50.count(), 1000);
	BOOST_CHECK_EQUAL(snapshot.p99.count(), 1000);
	BOOST_CHECK_EQUAL(snapshot.min.count(), 1000);
	BOOST_CHECK_EQUAL(snapshot.max.count(), 1000);
	BOOST_CHECK_EQUAL(snapshot.alltime_min.count(), 1000);
	BOOST_CHECK_EQUAL(snapshot.alltime_max.count(), 1000);
	BOOST_CHECK_EQUAL(snapshot.mean.count(), 1000);
}

BOOST_AUTO_TEST_CASE(Test_MultipleSamples_CorrectMinMax)
{
	LatencyPercentileTracker<> tracker;
	tracker.Record(500ns);
	tracker.Record(1000ns);
	tracker.Record(1500ns);

	auto snapshot = tracker.GetSnapshot();

	BOOST_CHECK_EQUAL(snapshot.sample_count, 3);
	BOOST_CHECK_EQUAL(snapshot.min.count(), 500);
	BOOST_CHECK_EQUAL(snapshot.max.count(), 1500);
}

BOOST_AUTO_TEST_CASE(Test_MeanCalculation)
{
	LatencyPercentileTracker<> tracker;
	tracker.Record(100ns);
	tracker.Record(200ns);
	tracker.Record(300ns);

	auto snapshot = tracker.GetSnapshot();

	// Mean should be (100 + 200 + 300) / 3 = 200
	BOOST_CHECK_EQUAL(snapshot.mean.count(), 200);
}

//-----------------------------------------------------------------------------
// PERCENTILE CALCULATIONS
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_P50_IsMedian)
{
	LatencyPercentileTracker<> tracker;

	// Add 100 samples: 1, 2, 3, ..., 100 nanoseconds
	for (int i = 1; i <= 100; ++i)
	{
		tracker.Record(std::chrono::nanoseconds(i));
	}

	auto snapshot = tracker.GetSnapshot();

	// For 100 samples, p50 should be around 50-51 (depending on interpolation)
	BOOST_CHECK_GE(snapshot.p50.count(), 49);
	BOOST_CHECK_LE(snapshot.p50.count(), 52);
}

BOOST_AUTO_TEST_CASE(Test_P99_IsNear99thPercentile)
{
	LatencyPercentileTracker<> tracker;

	// Add 100 samples: 1, 2, 3, ..., 100 nanoseconds
	for (int i = 1; i <= 100; ++i)
	{
		tracker.Record(std::chrono::nanoseconds(i));
	}

	auto snapshot = tracker.GetSnapshot();

	// For 100 samples, p99 should be around 99
	BOOST_CHECK_GE(snapshot.p99.count(), 97);
	BOOST_CHECK_LE(snapshot.p99.count(), 100);
}

BOOST_AUTO_TEST_CASE(Test_P1_IsNear1stPercentile)
{
	LatencyPercentileTracker<> tracker;

	// Add 100 samples: 1, 2, 3, ..., 100 nanoseconds
	for (int i = 1; i <= 100; ++i)
	{
		tracker.Record(std::chrono::nanoseconds(i));
	}

	auto snapshot = tracker.GetSnapshot();

	// For 100 samples, p1 should be around 1-2
	BOOST_CHECK_GE(snapshot.p1.count(), 1);
	BOOST_CHECK_LE(snapshot.p1.count(), 3);
}

//-----------------------------------------------------------------------------
// SAMPLE LIMIT
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_MaxSampleLimit_EnforcedCorrectly)
{
	// Create tracker with max 10 samples
	LatencyPercentileTracker<> tracker(10, 60s);

	// Add 20 samples
	for (int i = 0; i < 20; ++i)
	{
		tracker.Record(std::chrono::nanoseconds(i * 100));
	}

	auto snapshot = tracker.GetSnapshot();

	// Should only have 10 samples (the latest ones)
	BOOST_CHECK_EQUAL(snapshot.sample_count, 10);

	// Min should be the 10th sample from the end (1000ns)
	BOOST_CHECK_EQUAL(snapshot.min.count(), 1000);

	// Max should be the last sample (1900ns)
	BOOST_CHECK_EQUAL(snapshot.max.count(), 1900);

	// All-time min/max should reflect the full range including evicted samples
	BOOST_CHECK_EQUAL(snapshot.alltime_min.count(), 0);
	BOOST_CHECK_EQUAL(snapshot.alltime_max.count(), 1900);
}

BOOST_AUTO_TEST_CASE(Test_AlltimeMinMax_SurviveWindowEviction)
{
	// Create tracker with max 5 samples
	LatencyPercentileTracker<> tracker(5, 60s);

	// Record an extreme low and high
	tracker.Record(10ns);
	tracker.Record(50000ns);

	// Fill the window with mid-range values to evict the extremes
	for (int i = 0; i < 5; ++i)
	{
		tracker.Record(std::chrono::nanoseconds(1000 + i * 100));
	}

	auto snapshot = tracker.GetSnapshot();

	// Window min/max should reflect only the retained mid-range samples
	BOOST_CHECK_EQUAL(snapshot.min.count(), 1000);
	BOOST_CHECK_EQUAL(snapshot.max.count(), 1400);

	// All-time min/max should still reflect the original extremes
	BOOST_CHECK_EQUAL(snapshot.alltime_min.count(), 10);
	BOOST_CHECK_EQUAL(snapshot.alltime_max.count(), 50000);
}

//-----------------------------------------------------------------------------
// RESET FUNCTIONALITY
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_Reset_ClearsAllSamples)
{
	LatencyPercentileTracker<> tracker;
	tracker.Record(1000ns);
	tracker.Record(2000ns);

	BOOST_CHECK_EQUAL(tracker.TotalSampleCount(), 2);

	tracker.Reset();

	auto snapshot = tracker.GetSnapshot();
	BOOST_CHECK_EQUAL(snapshot.sample_count, 0);
	BOOST_CHECK_EQUAL(snapshot.alltime_min.count(), 0);
	BOOST_CHECK_EQUAL(snapshot.alltime_max.count(), 0);
	BOOST_CHECK_EQUAL(tracker.TotalSampleCount(), 0);
}

//-----------------------------------------------------------------------------
// RECORD SINCE FUNCTIONALITY
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_RecordSince_RecordsCorrectLatency)
{
	LatencyPercentileTracker<> tracker;

	auto start = std::chrono::steady_clock::now();
	// Small delay to ensure measurable latency
	std::this_thread::sleep_for(1ms);
	tracker.RecordSince(start);

	auto snapshot = tracker.GetSnapshot();

	// Should have recorded at least 1ms (1000000ns)
	BOOST_CHECK_GE(snapshot.p50.count(), 1000000);
}

//-----------------------------------------------------------------------------
// SCOPED MEASUREMENT
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_ScopedMeasurement_RecordsOnDestruction)
{
	LatencyPercentileTracker<> tracker;

	{
		ScopedLatencyMeasurement<> measurement(tracker);
		std::this_thread::sleep_for(1ms);
	}

	auto snapshot = tracker.GetSnapshot();

	BOOST_CHECK_EQUAL(snapshot.sample_count, 1);
	BOOST_CHECK_GE(snapshot.p50.count(), 1000000); // At least 1ms
}

BOOST_AUTO_TEST_CASE(Test_ScopedMeasurement_CancelledDoesNotRecord)
{
	LatencyPercentileTracker<> tracker;

	{
		ScopedLatencyMeasurement<> measurement(tracker);
		measurement.Cancel();
		std::this_thread::sleep_for(1ms);
	}

	auto snapshot = tracker.GetSnapshot();

	BOOST_CHECK_EQUAL(snapshot.sample_count, 0);
}

//-----------------------------------------------------------------------------
// THREAD SAFETY
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_ConcurrentAccess_NoDataRace)
{
	LatencyPercentileTracker<> tracker(1000, 60s);

	std::vector<std::thread> threads;

	// Writer threads
	for (int t = 0; t < 4; ++t)
	{
		threads.emplace_back([&tracker, t]()
		{
			for (int i = 0; i < 100; ++i)
			{
				tracker.Record(std::chrono::nanoseconds((t * 100) + i));
			}
		});
	}

	// Reader thread
	threads.emplace_back([&tracker]()
	{
		for (int i = 0; i < 50; ++i)
		{
			auto snapshot = tracker.GetSnapshot();
			(void)snapshot.sample_count; // Use the result to prevent optimization
		}
	});

	for (auto& thread : threads)
	{
		thread.join();
	}

	// If we get here without crashing/hanging, the test passes
	BOOST_CHECK_GE(tracker.TotalSampleCount(), 1);
}

//-----------------------------------------------------------------------------
// SERIAL LATENCY METRICS COLLECTION
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_SerialLatencyMetrics_HasAllTrackers)
{
	SerialLatencyMetrics metrics;

	// Record some data to each tracker
	metrics.ReadLatency.Record(1000ns);
	metrics.WriteLatency.Record(2000ns);
	metrics.MessageProcessingLatency.Record(3000ns);

	// Verify each tracker has data
	BOOST_CHECK_EQUAL(metrics.ReadLatency.GetSnapshot().sample_count, 1);
	BOOST_CHECK_EQUAL(metrics.WriteLatency.GetSnapshot().sample_count, 1);
	BOOST_CHECK_EQUAL(metrics.MessageProcessingLatency.GetSnapshot().sample_count, 1);
}

BOOST_AUTO_TEST_SUITE_END()
