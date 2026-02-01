#include <chrono>
#include <random>
#include <thread>

#include <benchmark/benchmark.h>

#include "utility/latency_percentile_tracker.h"

using namespace AqualinkAutomate::Utility;

//-----------------------------------------------------------------------------
// LATENCY TRACKER - RECORD OPERATIONS
//-----------------------------------------------------------------------------

class LatencyTracker_Record_Fixture : public benchmark::Fixture
{
public:
	void SetUp(const ::benchmark::State& state) override
	{
		tracker = std::make_unique<LatencyPercentileTracker<>>(1000, std::chrono::seconds(60));

		// Pre-generate random latencies
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int64_t> dist(100, 100000); // 100ns to 100us

		random_latencies.resize(10000);
		for (auto& latency : random_latencies)
		{
			latency = std::chrono::nanoseconds(dist(gen));
		}
	}

	void TearDown(const ::benchmark::State&) override
	{
		tracker.reset();
	}

public:
	std::unique_ptr<LatencyPercentileTracker<>> tracker;
	std::vector<std::chrono::nanoseconds> random_latencies;
};

// Benchmark the cost of recording a single latency sample
BENCHMARK_DEFINE_F(LatencyTracker_Record_Fixture, Record_SingleSample)(benchmark::State& state)
{
	size_t index = 0;
	for (auto _ : state)
	{
		tracker->Record(random_latencies[index % random_latencies.size()]);
		index++;
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(LatencyTracker_Record_Fixture, Record_SingleSample)->Unit(benchmark::kNanosecond);

// Benchmark recording latency under high load (many samples)
BENCHMARK_DEFINE_F(LatencyTracker_Record_Fixture, Record_HighLoad)(benchmark::State& state)
{
	size_t samples_per_iteration = state.range(0);

	for (auto _ : state)
	{
		for (size_t i = 0; i < samples_per_iteration; ++i)
		{
			tracker->Record(random_latencies[i % random_latencies.size()]);
		}
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations() * samples_per_iteration);
}
BENCHMARK_REGISTER_F(LatencyTracker_Record_Fixture, Record_HighLoad)
	->Arg(100)
	->Arg(500)
	->Arg(1000)
	->Unit(benchmark::kMicrosecond);

//-----------------------------------------------------------------------------
// LATENCY TRACKER - SNAPSHOT/PERCENTILE COMPUTATION
//-----------------------------------------------------------------------------

class LatencyTracker_Snapshot_Fixture : public benchmark::Fixture
{
public:
	void SetUp(const ::benchmark::State& state) override
	{
		tracker = std::make_unique<LatencyPercentileTracker<>>(static_cast<size_t>(state.range(0)), std::chrono::seconds(60));

		// Pre-populate with samples
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int64_t> dist(100, 100000);

		size_t sample_count = static_cast<size_t>(state.range(0));
		for (size_t i = 0; i < sample_count; ++i)
		{
			tracker->Record(std::chrono::nanoseconds(dist(gen)));
		}
	}

	void TearDown(const ::benchmark::State&) override
	{
		tracker.reset();
	}

public:
	std::unique_ptr<LatencyPercentileTracker<>> tracker;
};

// Benchmark the cost of computing a percentile snapshot
BENCHMARK_DEFINE_F(LatencyTracker_Snapshot_Fixture, GetSnapshot)(benchmark::State& state)
{
	for (auto _ : state)
	{
		auto snapshot = tracker->GetSnapshot();
		benchmark::DoNotOptimize(snapshot);
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(LatencyTracker_Snapshot_Fixture, GetSnapshot)
	->Arg(100)
	->Arg(500)
	->Arg(1000)
	->Unit(benchmark::kMicrosecond);

//-----------------------------------------------------------------------------
// SCOPED LATENCY MEASUREMENT
//-----------------------------------------------------------------------------

class ScopedMeasurement_Fixture : public benchmark::Fixture
{
public:
	void SetUp(const ::benchmark::State&) override
	{
		tracker = std::make_unique<LatencyPercentileTracker<>>(1000, std::chrono::seconds(60));
	}

	void TearDown(const ::benchmark::State&) override
	{
		tracker.reset();
	}

public:
	std::unique_ptr<LatencyPercentileTracker<>> tracker;
};

// Benchmark the overhead of scoped measurement (RAII pattern)
BENCHMARK_DEFINE_F(ScopedMeasurement_Fixture, ScopedMeasurement_Overhead)(benchmark::State& state)
{
	for (auto _ : state)
	{
		ScopedLatencyMeasurement measurement(*tracker);
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(ScopedMeasurement_Fixture, ScopedMeasurement_Overhead)->Unit(benchmark::kNanosecond);

// Benchmark scoped measurement with cancelled operation (no recording)
BENCHMARK_DEFINE_F(ScopedMeasurement_Fixture, ScopedMeasurement_Cancelled)(benchmark::State& state)
{
	for (auto _ : state)
	{
		ScopedLatencyMeasurement measurement(*tracker);
		measurement.Cancel();
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(ScopedMeasurement_Fixture, ScopedMeasurement_Cancelled)->Unit(benchmark::kNanosecond);

//-----------------------------------------------------------------------------
// CONCURRENT ACCESS PERFORMANCE
//-----------------------------------------------------------------------------

// Note: This tests single-threaded performance of the thread-safe implementation.
// The mutex overhead is included in all measurements above.

class LatencyTracker_Concurrent_Fixture : public benchmark::Fixture
{
public:
	void SetUp(const ::benchmark::State&) override
	{
		tracker = std::make_unique<LatencyPercentileTracker<>>(1000, std::chrono::seconds(60));

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int64_t> dist(100, 100000);

		random_latencies.resize(1000);
		for (auto& latency : random_latencies)
		{
			latency = std::chrono::nanoseconds(dist(gen));
		}
	}

	void TearDown(const ::benchmark::State&) override
	{
		tracker.reset();
	}

public:
	std::unique_ptr<LatencyPercentileTracker<>> tracker;
	std::vector<std::chrono::nanoseconds> random_latencies;
};

// Benchmark interleaved Record/GetSnapshot operations (simulates real usage)
BENCHMARK_DEFINE_F(LatencyTracker_Concurrent_Fixture, InterleavedOperations)(benchmark::State& state)
{
	size_t index = 0;
	for (auto _ : state)
	{
		// 10 records for every snapshot
		for (int i = 0; i < 10; ++i)
		{
			tracker->Record(random_latencies[index % random_latencies.size()]);
			index++;
		}

		auto snapshot = tracker->GetSnapshot();
		benchmark::DoNotOptimize(snapshot);
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations() * 11); // 10 records + 1 snapshot
}
BENCHMARK_REGISTER_F(LatencyTracker_Concurrent_Fixture, InterleavedOperations)->Unit(benchmark::kMicrosecond);

//-----------------------------------------------------------------------------
// MEMORY EFFICIENCY - Sample Window Pruning
//-----------------------------------------------------------------------------

class LatencyTracker_Pruning_Fixture : public benchmark::Fixture
{
public:
	void SetUp(const ::benchmark::State& state) override
	{
		// Use a very short window to trigger frequent pruning
		tracker = std::make_unique<LatencyPercentileTracker<>>(
			static_cast<size_t>(state.range(0)),
			std::chrono::seconds(1));

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int64_t> dist(100, 100000);

		random_latencies.resize(1000);
		for (auto& latency : random_latencies)
		{
			latency = std::chrono::nanoseconds(dist(gen));
		}
	}

	void TearDown(const ::benchmark::State&) override
	{
		tracker.reset();
	}

public:
	std::unique_ptr<LatencyPercentileTracker<>> tracker;
	std::vector<std::chrono::nanoseconds> random_latencies;
};

// Benchmark recording with aggressive pruning
BENCHMARK_DEFINE_F(LatencyTracker_Pruning_Fixture, RecordWithPruning)(benchmark::State& state)
{
	size_t index = 0;
	for (auto _ : state)
	{
		tracker->Record(random_latencies[index % random_latencies.size()]);
		index++;
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(LatencyTracker_Pruning_Fixture, RecordWithPruning)
	->Arg(100)
	->Arg(500)
	->Arg(1000)
	->Unit(benchmark::kNanosecond);
