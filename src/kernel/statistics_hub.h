#pragma once

#include <cstdint>

#include "interfaces/ihub.h"
#include "jandy/messages/jandy_message_ids.h"
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

		uint32_t MessageErrorRate;
		uint32_t BandwidthUtilisation;
		uint32_t AvgMinMaxMessageSizes;
		uint32_t SerialOverflowCount;
		uint32_t SerialUnderflowCount;
		uint32_t TransmissionFailures;
		uint32_t SerialWriteQueueDepth;
	};

}
// namespace AqualinkAutomate::Kernel
