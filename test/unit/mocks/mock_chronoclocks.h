#pragma once

#include <chrono>

#include "developer/chrono_clocks.h"

using namespace AqualinkAutomate;

namespace AqualinkAutomate::Test
{

	struct MockChronoClocks
	{
		struct SteadyClock : std::chrono::steady_clock
		{
			using base_type = std::chrono::steady_clock;

			// Enable injection and manipulation of "now".

			static time_point next_time_point;
			static void set_next_time_point(time_point tp);
			static time_point now();
		};
	};

}
// namespace AqualinkAutomate::Test
