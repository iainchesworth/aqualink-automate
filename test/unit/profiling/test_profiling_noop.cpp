#include <boost/test/unit_test.hpp>

#include "profiling/noop_profiler.h"

using namespace AqualinkAutomate::Profiling;

BOOST_AUTO_TEST_SUITE(NoOpProfiler_TestSuite)

// =============================================================================
// Construction
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConstruction_DoesNotThrow)
{
	BOOST_CHECK_NO_THROW(NoOp_Profiler profiler);
}

// =============================================================================
// StartProfiling / StopProfiling are no-ops
// =============================================================================

BOOST_AUTO_TEST_CASE(TestStartProfiling_DoesNotThrow)
{
	NoOp_Profiler profiler;
	BOOST_CHECK_NO_THROW(profiler.StartProfiling());
}

BOOST_AUTO_TEST_CASE(TestStopProfiling_DoesNotThrow)
{
	NoOp_Profiler profiler;
	BOOST_CHECK_NO_THROW(profiler.StopProfiling());
}

BOOST_AUTO_TEST_CASE(TestStartThenStop_DoesNotThrow)
{
	NoOp_Profiler profiler;
	BOOST_CHECK_NO_THROW(profiler.StartProfiling());
	BOOST_CHECK_NO_THROW(profiler.StopProfiling());
}

BOOST_AUTO_TEST_CASE(TestMultipleStartStop_DoesNotThrow)
{
	NoOp_Profiler profiler;
	for (int i = 0; i < 5; ++i)
	{
		BOOST_CHECK_NO_THROW(profiler.StartProfiling());
		BOOST_CHECK_NO_THROW(profiler.StopProfiling());
	}
}

BOOST_AUTO_TEST_SUITE_END()
