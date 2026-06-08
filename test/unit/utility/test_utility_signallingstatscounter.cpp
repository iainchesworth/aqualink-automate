#include <sstream>
#include <string>
#include <type_traits>

#include <boost/test/unit_test.hpp>

#include <magic_enum/magic_enum.hpp>

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

BOOST_AUTO_TEST_CASE(StatsCounter_CopyAssignment_IsDeleted)
{
    // StatsCounter copy/move assignment is deleted because m_StatsSignal is a reference.
    BOOST_CHECK(!std::is_copy_assignable_v<AqualinkAutomate::Utility::StatsCounter>);
    BOOST_CHECK(!std::is_move_assignable_v<AqualinkAutomate::Utility::StatsCounter>);
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

// Regression: repeated operator[] on the same stat must resolve to the SAME stored
// counter (single insertion) and must NOT insert a duplicate entry. This guards the
// single-lookup (try_emplace + heterogeneous find) rewrite.
BOOST_AUTO_TEST_CASE(SignallingStatsCounter_RepeatedAccessReturnsSameEntry)
{
    AqualinkAutomate::Utility::SignallingStatsCounter counter;

    counter[TestStatA::Stat1] = 7;

    // Mutating through a second access must observe the original value (same entry).
    counter[TestStatA::Stat1] += 3;
    BOOST_CHECK_EQUAL(counter[TestStatA::Stat1].Count(), 10);

    // Hot-path repeated lookups must not create extra entries.
    for (int i = 0; i < 16; ++i)
    {
        (void)counter[TestStatA::Stat1];
    }

    int entries = 0;
    for (auto it = counter.cbegin(); it != counter.cend(); ++it)
    {
        ++entries;
    }
    BOOST_CHECK_EQUAL(entries, 1);
}

// Regression: distinct enum values of the same type AND values across different enum
// types must each get their own counter (type_hash + value_hash key).
BOOST_AUTO_TEST_CASE(SignallingStatsCounter_DistinctKeysAcrossTypes)
{
    AqualinkAutomate::Utility::SignallingStatsCounter counter;

    counter[TestStatA::Stat1] = 1;
    counter[TestStatA::Stat2] = 2;
    counter[TestStatA::Stat3] = 3;
    counter[TestStatB::Alpha] = 10;
    counter[TestStatB::Beta] = 20;

    BOOST_CHECK_EQUAL(counter[TestStatA::Stat1].Count(), 1);
    BOOST_CHECK_EQUAL(counter[TestStatA::Stat2].Count(), 2);
    BOOST_CHECK_EQUAL(counter[TestStatA::Stat3].Count(), 3);
    BOOST_CHECK_EQUAL(counter[TestStatB::Alpha].Count(), 10);
    BOOST_CHECK_EQUAL(counter[TestStatB::Beta].Count(), 20);

    int entries = 0;
    for (auto it = counter.cbegin(); it != counter.cend(); ++it)
    {
        ++entries;
    }
    BOOST_CHECK_EQUAL(entries, 5);
}

// Regression: AddStatsToCount must create the counter (lazily) and must NOT throw on a
// repeat call for an already-present stat (the previous emplace-failure throw path is
// gone now that operator[] does a single guarded insert).
BOOST_AUTO_TEST_CASE(SignallingStatsCounter_AddStatsToCountIsIdempotent)
{
    AqualinkAutomate::Utility::SignallingStatsCounter counter;

    BOOST_CHECK_NO_THROW(counter.AddStatsToCount(TestStatA::Stat1));
    BOOST_CHECK_NO_THROW(counter.AddStatsToCount(TestStatA::Stat1));

    BOOST_CHECK_EQUAL(counter[TestStatA::Stat1].Count(), 0);

    int entries = 0;
    for (auto it = counter.cbegin(); it != counter.cend(); ++it)
    {
        ++entries;
    }
    BOOST_CHECK_EQUAL(entries, 1);
}

// Regression: the stored key must still expose a populated display name for iteration
// consumers (e.g. the equipment-stats JSON serialiser reads `msg_id.name`). This is the
// contract that survives the removal of the per-lookup throwaway name string.
BOOST_AUTO_TEST_CASE(SignallingStatsCounter_StoredKeyExposesDisplayName)
{
    AqualinkAutomate::Utility::SignallingStatsCounter counter;

    counter[TestStatA::Stat2] = 1;

    bool found = false;
    for ([[maybe_unused]] const auto& [key, value] : counter)
    {
        found = true;
        BOOST_CHECK_EQUAL(key.name, std::string(magic_enum::enum_name(TestStatA::Stat2)));
        BOOST_CHECK(!key.name.empty());
    }
    BOOST_CHECK(found);
}

BOOST_AUTO_TEST_SUITE_END();
