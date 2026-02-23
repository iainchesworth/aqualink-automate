#include <format>
#include <sstream>
#include <string>

#include <boost/test/unit_test.hpp>

#include "jandy/formatters/stats_counter_formatter.h"
#include "core/utility/signalling_stats_counter.h"

using namespace AqualinkAutomate::Utility;

BOOST_AUTO_TEST_SUITE(Formatters_StatsCounterTestSuite)

// =============================================================================
// Default StatsCounter
// =============================================================================

BOOST_AUTO_TEST_CASE(TestFormat_DefaultStatsCounter_FormatsAsZero)
{
	StatsSignal signal;
	StatsCounter counter(signal);
	auto result = std::format("{}", counter);
	BOOST_CHECK_EQUAL(result, "0");
}

// =============================================================================
// After Increments
// =============================================================================

BOOST_AUTO_TEST_CASE(TestFormat_AfterIncrements_FormatsAsN)
{
	StatsSignal signal;
	StatsCounter counter(signal);
	++counter;
	++counter;
	++counter;
	auto result = std::format("{}", counter);
	BOOST_CHECK_EQUAL(result, "3");
}

BOOST_AUTO_TEST_CASE(TestFormat_AfterAssignment)
{
	StatsSignal signal;
	StatsCounter counter(signal);
	counter = 42;
	auto result = std::format("{}", counter);
	BOOST_CHECK_EQUAL(result, "42");
}

// =============================================================================
// ostream operator
// =============================================================================

BOOST_AUTO_TEST_CASE(TestOstream_StatsCounter_MatchesFormat)
{
	StatsSignal signal;
	StatsCounter counter(signal);
	counter = 99;

	std::ostringstream oss;
	oss << counter;

	BOOST_CHECK_EQUAL(oss.str(), std::format("{}", counter));
	BOOST_CHECK_EQUAL(oss.str(), "99");
}

BOOST_AUTO_TEST_SUITE_END()
