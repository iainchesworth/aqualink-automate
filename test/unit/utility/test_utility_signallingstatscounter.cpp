#include <sstream>

#include <boost/test/unit_test.hpp>

#include "jandy/formatters/stats_counter_formatter.h"
#include "utility/signalling_stats_counter.h"

BOOST_AUTO_TEST_SUITE(SignallingStatsCounter);

BOOST_AUTO_TEST_CASE(CountTest)
{
    AqualinkAutomate::Utility::StatsSignal signal;
    AqualinkAutomate::Utility::StatsCounter counter(signal);

    // Test initial count is 0
    BOOST_TEST(counter.Count() == 0);

    // Test count after assignment
    counter = 5;
    BOOST_TEST(counter.Count() == 5);

    // Test count after addition
    counter += 3;
    BOOST_TEST(counter.Count() == 8);

    // Test count after pre-increment
    ++counter;
    BOOST_TEST(counter.Count() == 9);

    // Test count after post-increment
    counter++;
    BOOST_TEST(counter.Count() == 10);
}

BOOST_AUTO_TEST_CASE(StatsSignalTest)
{
    AqualinkAutomate::Utility::StatsSignal signal;

    bool signalReceived = false;

    signal.connect([&signalReceived]() { signalReceived = true; });

    AqualinkAutomate::Utility::StatsCounter counter(signal);

    // Trigger the signal by assigning a new value
    counter = 5;

    // Check if the signal was received
    BOOST_TEST(signalReceived);
}

BOOST_AUTO_TEST_CASE(StatsCounterAccessTest)
{
    using CounterType = AqualinkAutomate::Utility::SignallingStatsCounter<std::string, int>;

    CounterType counter;

    // Test accessing and modifying stats counters
    counter["stat1"] += 5;
    counter[42] = 10;

    BOOST_TEST(counter["stat1"].Count() == 5);
    BOOST_TEST(counter[42].Count() == 10);
}

BOOST_AUTO_TEST_CASE(VariantStatsSignalTest)
{
    using CounterType = AqualinkAutomate::Utility::SignallingStatsCounter<double>;

    CounterType counter;

    bool signalReceived = false;
    
    counter.Signal().connect([&signalReceived]() { signalReceived = true; });

    // Trigger the signal by accessing a new stats counter
    counter[3.14] += 1;

    // Check if the signal was received
    BOOST_TEST(signalReceived);
}

BOOST_AUTO_TEST_CASE(OstreamOperatorNormalCase)
{
    AqualinkAutomate::Utility::StatsSignal signal;
    AqualinkAutomate::Utility::StatsCounter counter(signal);

    {
        std::ostringstream oss;
        oss << counter;
        std::string result = oss.str();
        BOOST_CHECK_EQUAL("0", result);
    }

    counter = 5;
    {
        std::ostringstream oss;
        oss << counter;
        std::string result = oss.str();
        BOOST_CHECK_EQUAL("5", result);
    }

    counter += 3;
    {
        std::ostringstream oss;
        oss << counter;
        std::string result = oss.str();
        BOOST_CHECK_EQUAL("8", result);
    }

    ++counter;
    {
        std::ostringstream oss;
        oss << counter;
        std::string result = oss.str();
        BOOST_CHECK_EQUAL("9", result);
    }

    counter++;
    {
        std::ostringstream oss;
        oss << counter;
        std::string result = oss.str();
        BOOST_CHECK_EQUAL("10", result);
    }
}

BOOST_AUTO_TEST_CASE(FormatNormalCase)
{
    AqualinkAutomate::Utility::StatsSignal signal;
    AqualinkAutomate::Utility::StatsCounter counter(signal);

    BOOST_CHECK_EQUAL("0", std::format("{}", counter));

    counter = 5;
    BOOST_CHECK_EQUAL("5", std::format("{}", counter));

    counter += 3;
    BOOST_CHECK_EQUAL("8", std::format("{}", counter));

    ++counter;
    BOOST_CHECK_EQUAL("9", std::format("{}", counter));

    counter++;
    BOOST_CHECK_EQUAL("10", std::format("{}", counter));
}

BOOST_AUTO_TEST_SUITE_END();
