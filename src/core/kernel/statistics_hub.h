#pragma once

#include <chrono>
#include <cstdint>
#include <memory>

#include "interfaces/ihub.h"
#include "utility/bandwidth_utilisation.h"
#include "utility/latency_percentile_tracker.h"
#include "utility/signalling_stats_counter.h"

namespace AqualinkAutomate::Kernel
{

	class StatisticsHub : public Interfaces::IHub
	{
	public:
		StatisticsHub();
		~StatisticsHub() override = default;

	public:
		// The total number of messages sent or received.
		// This is a generic counter that works with any enum type (e.g., JandyMessageIds, PentairMessageIds).
		// Use ++MessageCounts[SomeMessageIdEnum::Value] to increment.
		Utility::SignallingStatsCounter MessageCounts;

	public:
		Utility::BandwidthMetricsCollection BandwidthMetrics;

	public:
		// Latency metrics for serial and message processing operations.
		// These track percentiles (p1, p50, p95, p99) in real-time.
		Utility::SerialLatencyMetrics LatencyMetrics;

	public:
		struct SerialMetrics
		{
			uint64_t MessageErrorRate{ 0 };
			uint64_t AvgMinMaxMessageSizes{ 0 };
			uint64_t SerialOverflowCount{ 0 };
			uint64_t SerialUnderflowCount{ 0 };
			uint64_t TransmissionFailures{ 0 };
			uint64_t SerialWriteQueueDepth{ 0 };
		};

		SerialMetrics Serial;

	public:
		struct MessageErrorMetrics
		{
			uint64_t ChecksumFailures{ 0 };
			uint64_t DeserializationFailures{ 0 };
			uint64_t InvalidPacketFormat{ 0 };
			uint64_t GeneratorFailures{ 0 };
			uint64_t OverlappingPackets{ 0 };
			uint64_t BufferOverflows{ 0 };
		};

		MessageErrorMetrics MessageErrors;
	};

}
// namespace AqualinkAutomate::Kernel
