#pragma once

#include "jandy/messages/jandy_message_ids.h"
#include "utility/signalling_stats_counter.h"

namespace AqualinkAutomate::Kernel
{

	class StatisticsHub
	{
	public:
		Utility::SignallingStatsCounter<Messages::JandyMessageIds> Messages;
	};

}
// namespace AqualinkAutomate::Kernel
