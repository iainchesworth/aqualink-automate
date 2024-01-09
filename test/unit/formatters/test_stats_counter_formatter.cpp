#include <sstream>

#include <boost/test/unit_test.hpp>

#include "jandy/formatters/stats_counter_formatter.h"

BOOST_AUTO_TEST_SUITE(StatsCounterFormatter)

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

BOOST_AUTO_TEST_SUITE_END()
