#include "mocks/mock_chronoclocks.h"

namespace AqualinkAutomate::Test
{
	MockChronoClocks::SteadyClock::time_point MockChronoClocks::SteadyClock::next_time_point;

	void MockChronoClocks::SteadyClock::set_next_time_point(time_point tp)
	{
		next_time_point = tp;
	}

	MockChronoClocks::SteadyClock::time_point MockChronoClocks::SteadyClock::now()
	{
		if (0 != next_time_point.time_since_epoch().count())
		{
			return next_time_point;
		}

		return base_type::now();
	}

}
// namespace AqualinkAutomate::Test
