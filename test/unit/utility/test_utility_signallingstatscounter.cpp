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

    signal.connect([&signalReceived](uint64_t /* ignored */) { signalReceived = true; });

    AqualinkAutomate::Utility::StatsCounter counter(signal);

    // Trigger the signal by assigning a new value
    counter = 5;

    // Check if the signal was received
    BOOST_TEST(signalReceived);
}

enum class TestStatA { Stat1, Stat2, Stat3 };
enum class TestStatB { Alpha, Beta };

BOOST_AUTO_TEST_CASE(StatsCounterAccessTest)
{
    AqualinkAutomate::Utility::SignallingStatsCounter counter;

    // Test accessing and modifying stats counters
    counter[TestStatA::Stat1] += 5;
    counter[TestStatA::Stat2] = 10;

    BOOST_TEST(counter[TestStatA::Stat1].Count() == 5);
    BOOST_TEST(counter[TestStatA::Stat2].Count() == 10);
}

BOOST_AUTO_TEST_CASE(VariantStatsSignalTest)
{
    AqualinkAutomate::Utility::SignallingStatsCounter counter;

    bool signalReceived = false;

    counter.Signal().connect([&signalReceived](uint64_t /* ignored */) { signalReceived = true; });

    // Trigger the signal by accessing a new stats counter
    counter[TestStatB::Alpha] += 1;

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

BOOST_AUTO_TEST_CASE(StatsCounter_CopyConstructor)
{
    AqualinkAutomate::Utility::StatsSignal signal;
    AqualinkAutomate::Utility::StatsCounter original(signal);
    original = 42;

    AqualinkAutomate::Utility::StatsCounter copy(original);
    BOOST_CHECK_EQUAL(copy.Count(), 42);
}

BOOST_AUTO_TEST_CASE(StatsCounter_MoveConstructor)
{
    AqualinkAutomate::Utility::StatsSignal signal;
    AqualinkAutomate::Utility::StatsCounter original(signal);
    original = 99;

    AqualinkAutomate::Utility::StatsCounter moved(std::move(original));
    BOOST_CHECK_EQUAL(moved.Count(), 99);
}

BOOST_AUTO_TEST_CASE(StatsCounter_CopyAssignment)
{
    AqualinkAutomate::Utility::StatsSignal signal;
    AqualinkAutomate::Utility::StatsCounter original(signal);
    original = 55;

    AqualinkAutomate::Utility::StatsCounter other(signal);
    other = original;
    BOOST_CHECK_EQUAL(other.Count(), 55);
}

BOOST_AUTO_TEST_CASE(StatsCounter_MoveAssignment)
{
    AqualinkAutomate::Utility::StatsSignal signal;
    AqualinkAutomate::Utility::StatsCounter original(signal);
    original = 77;

    AqualinkAutomate::Utility::StatsCounter other(signal);
    other = std::move(original);
    BOOST_CHECK_EQUAL(other.Count(), 77);
}

BOOST_AUTO_TEST_CASE(StatsCounter_OperatorCallConst)
{
    AqualinkAutomate::Utility::StatsSignal signal;
    AqualinkAutomate::Utility::StatsCounter counter(signal);
    counter = 123;

    const auto& const_counter = counter;
    BOOST_CHECK_EQUAL(const_counter(), 123);
}

BOOST_AUTO_TEST_CASE(SignallingStatsCounter_BeginEnd)
{
    AqualinkAutomate::Utility::SignallingStatsCounter counter;

    counter[TestStatA::Stat1] = 10;
    counter[TestStatA::Stat2] = 20;

    int count = 0;
    for (auto it = counter.begin(); it != counter.end(); ++it)
    {
        ++count;
    }
    BOOST_CHECK_EQUAL(count, 2);
}

BOOST_AUTO_TEST_CASE(SignallingStatsCounter_CBeginCEnd)
{
    AqualinkAutomate::Utility::SignallingStatsCounter counter;

    counter[TestStatB::Alpha] = 5;

    int count = 0;
    for (auto it = counter.cbegin(); it != counter.cend(); ++it)
    {
        ++count;
    }
    BOOST_CHECK_EQUAL(count, 1);
}

BOOST_AUTO_TEST_SUITE_END();
