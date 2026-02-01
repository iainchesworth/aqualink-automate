#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <vector>

namespace AqualinkAutomate::Utility
{

	/// Tracks latency samples and computes percentiles (p1, p50, p99) in real-time.
	/// Uses a sliding window approach to maintain recent samples for accurate percentile computation.
	/// Thread-safe for concurrent access.
	template<typename CLOCK_TYPE = std::chrono::steady_clock>
	class LatencyPercentileTracker
	{
	public:
		using Duration = std::chrono::nanoseconds;
		using TimePoint = typename CLOCK_TYPE::time_point;

		struct LatencySample
		{
			TimePoint timestamp;
			Duration latency;
		};

		struct PercentileSnapshot
		{
			Duration p1{ 0 };    // 1st percentile (best case)
			Duration p50{ 0 };   // Median
			Duration p95{ 0 };   // 95th percentile
			Duration p99{ 0 };   // 99th percentile (worst case)
			Duration min{ 0 };   // Minimum observed
			Duration max{ 0 };   // Maximum observed
			Duration mean{ 0 };  // Arithmetic mean
			std::size_t sample_count{ 0 };
		};

	public:
		/// Constructs a latency tracker with the specified window size and duration.
		/// @param max_samples Maximum number of samples to retain (default 1000).
		/// @param window_duration Only samples within this duration are considered (default 60 seconds).
		explicit LatencyPercentileTracker(
			std::size_t max_samples = 1000,
			std::chrono::seconds window_duration = std::chrono::seconds(60))
			: m_MaxSamples(max_samples)
			, m_WindowDuration(window_duration)
		{
		}

		~LatencyPercentileTracker() = default;

		// Non-copyable but movable
		LatencyPercentileTracker(const LatencyPercentileTracker&) = delete;
		LatencyPercentileTracker& operator=(const LatencyPercentileTracker&) = delete;
		LatencyPercentileTracker(LatencyPercentileTracker&&) = default;
		LatencyPercentileTracker& operator=(LatencyPercentileTracker&&) = default;

	public:
		/// Records a latency sample with the current timestamp.
		void Record(Duration latency)
		{
			std::lock_guard<std::mutex> lock(m_Mutex);

			auto now = CLOCK_TYPE::now();
			m_Samples.push_back(LatencySample{ now, latency });

			// Prune old samples
			PruneOldSamples(now);

			// Enforce max sample limit
			while (m_Samples.size() > m_MaxSamples)
			{
				m_Samples.pop_front();
			}

			// Update running statistics
			m_TotalSamples++;
			m_TotalLatency += latency;
		}

		/// Records a latency sample measured from a start time to now.
		void RecordSince(TimePoint start_time)
		{
			auto now = CLOCK_TYPE::now();
			auto latency = std::chrono::duration_cast<Duration>(now - start_time);
			Record(latency);
		}

		/// Computes and returns the current percentile snapshot.
		PercentileSnapshot GetSnapshot() const
		{
			std::lock_guard<std::mutex> lock(m_Mutex);

			PercentileSnapshot snapshot;

			if (m_Samples.empty())
			{
				return snapshot;
			}

			// Get samples within the window
			auto now = CLOCK_TYPE::now();
			std::vector<Duration> valid_samples;
			valid_samples.reserve(m_Samples.size());

			for (const auto& sample : m_Samples)
			{
				if ((now - sample.timestamp) <= m_WindowDuration)
				{
					valid_samples.push_back(sample.latency);
				}
			}

			if (valid_samples.empty())
			{
				return snapshot;
			}

			// Sort for percentile computation
			std::sort(valid_samples.begin(), valid_samples.end());

			snapshot.sample_count = valid_samples.size();
			snapshot.min = valid_samples.front();
			snapshot.max = valid_samples.back();

			// Compute percentiles
			snapshot.p1 = ComputePercentile(valid_samples, 1);
			snapshot.p50 = ComputePercentile(valid_samples, 50);
			snapshot.p95 = ComputePercentile(valid_samples, 95);
			snapshot.p99 = ComputePercentile(valid_samples, 99);

			// Compute mean
			Duration sum{ 0 };
			for (const auto& latency : valid_samples)
			{
				sum += latency;
			}
			snapshot.mean = sum / valid_samples.size();

			return snapshot;
		}

		/// Returns the total number of samples ever recorded.
		std::size_t TotalSampleCount() const
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			return m_TotalSamples;
		}

		/// Clears all recorded samples.
		void Reset()
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			m_Samples.clear();
			m_TotalSamples = 0;
			m_TotalLatency = Duration{ 0 };
		}

	private:
		void PruneOldSamples(TimePoint now)
		{
			while (!m_Samples.empty() && (now - m_Samples.front().timestamp) > m_WindowDuration)
			{
				m_Samples.pop_front();
			}
		}

		static Duration ComputePercentile(const std::vector<Duration>& sorted_samples, double percentile)
		{
			if (sorted_samples.empty())
			{
				return Duration{ 0 };
			}

			if (sorted_samples.size() == 1)
			{
				return sorted_samples[0];
			}

			// Linear interpolation for percentile computation
			double rank = (percentile / 100.0) * (sorted_samples.size() - 1);
			std::size_t lower_idx = static_cast<std::size_t>(std::floor(rank));
			std::size_t upper_idx = static_cast<std::size_t>(std::ceil(rank));
			double fraction = rank - lower_idx;

			if (lower_idx == upper_idx)
			{
				return sorted_samples[lower_idx];
			}

			// Linear interpolation between the two nearest values
			auto lower_val = sorted_samples[lower_idx].count();
			auto upper_val = sorted_samples[upper_idx].count();
			auto interpolated = lower_val + fraction * (upper_val - lower_val);

			return Duration{ static_cast<Duration::rep>(interpolated) };
		}

	private:
		mutable std::mutex m_Mutex;
		std::deque<LatencySample> m_Samples;
		std::size_t m_MaxSamples;
		std::chrono::seconds m_WindowDuration;

		// Running statistics
		std::size_t m_TotalSamples{ 0 };
		Duration m_TotalLatency{ 0 };
	};

	/// Convenience class for measuring operation latency using RAII.
	/// Records the latency when the scope exits.
	template<typename CLOCK_TYPE = std::chrono::steady_clock>
	class ScopedLatencyMeasurement
	{
	public:
		using Tracker = LatencyPercentileTracker<CLOCK_TYPE>;

		explicit ScopedLatencyMeasurement(Tracker& tracker)
			: m_Tracker(tracker)
			, m_StartTime(CLOCK_TYPE::now())
		{
		}

		~ScopedLatencyMeasurement()
		{
			if (!m_Cancelled)
			{
				m_Tracker.RecordSince(m_StartTime);
			}
		}

		// Non-copyable, non-movable
		ScopedLatencyMeasurement(const ScopedLatencyMeasurement&) = delete;
		ScopedLatencyMeasurement& operator=(const ScopedLatencyMeasurement&) = delete;
		ScopedLatencyMeasurement(ScopedLatencyMeasurement&&) = delete;
		ScopedLatencyMeasurement& operator=(ScopedLatencyMeasurement&&) = delete;

		/// Cancel recording - useful when the operation failed or was aborted.
		void Cancel()
		{
			m_Cancelled = true;
		}

	private:
		Tracker& m_Tracker;
		typename CLOCK_TYPE::time_point m_StartTime;
		bool m_Cancelled{ false };
	};

	/// Collection of latency metrics for serial operations.
	class SerialLatencyMetrics
	{
	public:
		SerialLatencyMetrics() = default;
		~SerialLatencyMetrics() = default;

		// Non-copyable, non-movable (contains mutexes)
		SerialLatencyMetrics(const SerialLatencyMetrics&) = delete;
		SerialLatencyMetrics& operator=(const SerialLatencyMetrics&) = delete;

	public:
		/// Latency for serial read operations (time from initiating read to receiving data).
		LatencyPercentileTracker<> ReadLatency{ 1000, std::chrono::seconds(60) };

		/// Latency for serial write operations (time from initiating write to completion).
		LatencyPercentileTracker<> WriteLatency{ 1000, std::chrono::seconds(60) };

		/// Latency for complete message processing (from receiving bytes to signaling handlers).
		LatencyPercentileTracker<> MessageProcessingLatency{ 1000, std::chrono::seconds(60) };

		/// Round-trip latency for command-response pairs.
		LatencyPercentileTracker<> RoundTripLatency{ 500, std::chrono::seconds(60) };
	};

}
// namespace AqualinkAutomate::Utility
