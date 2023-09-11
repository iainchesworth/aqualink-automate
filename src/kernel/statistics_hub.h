#pragma once

#include <chrono>
#include <cstdint>

#include "interfaces/ihub.h"
#include "jandy/messages/jandy_message_ids.h"
#include "utility/bandwidth_utilisation.h"
#include "utility/signalling_stats_counter.h"

namespace AqualinkAutomate::Kernel
{

	class StatisticsHub : public Interfaces::IHub
	{
	public:
		StatisticsHub();
		virtual ~StatisticsHub();

	public:
		// The total number of messages sent or received
		Utility::SignallingStatsCounter<Messages::JandyMessageIds> MessageCounts;

	public:
		Utility::BandwidthMetricsCollection BandwidthMetrics;

	public:
		struct SerialMetrics
		{
			uint64_t MessageErrorRate;
			uint64_t AvgMinMaxMessageSizes;
			uint64_t SerialOverflowCount;
			uint64_t SerialUnderflowCount;
			uint64_t TransmissionFailures;
			uint64_t SerialWriteQueueDepth;
		};
	};

}
// namespace AqualinkAutomate::Kernel
