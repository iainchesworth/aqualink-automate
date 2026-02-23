#pragma once

#include <chrono>

namespace AqualinkAutomate::Developer
{

	// The purpose of this struct is to enable injection of mocked clocks for
	// unit testing and other uses.

	struct ChronoClocks
	{
		using SteadyClock = std::chrono::steady_clock;
	};

}
// namespace AqualinkAutomate::Developer
