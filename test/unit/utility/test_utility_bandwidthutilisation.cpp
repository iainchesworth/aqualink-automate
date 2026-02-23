#include <boost/test/unit_test.hpp>

#include "utility/bandwidth_utilisation.h"

#include "mocks/mock_chronoclocks.h"

using BandwidthUtilisationMock = AqualinkAutomate::Utility::UtilisationOverPeriod<AqualinkAutomate::Test::MockChronoClocks::SteadyClock>;

BOOST_AUTO_TEST_SUITE(TestSuite_BandwidthUtilisation);

BOOST_AUTO_TEST_CASE(Test_BandwidthUtilisation_InitialUtilizationIsZero)
{
    BandwidthUtilisationMock bandwidth(std::chrono::seconds(1));

    BOOST_CHECK_CLOSE(bandwidth.Utilisation(), 0.0, 0.01f);
}

BOOST_AUTO_TEST_CASE(Test_BandwidthUtilisation_OperatorEqualTest)
{
    BandwidthUtilisationMock util1(std::chrono::seconds(1));

    util1 += 100;

    BandwidthUtilisationMock util2(std::chrono::seconds(2));

    util2 = util1;

    BOOST_CHECK(1 == util1.History().size());
    BOOST_CHECK(1 == util2.History().size());
    BOOST_CHECK(util1.History().size() == util2.History().size());
    BOOST_CHECK(util1.ReferenceTime() == util2.ReferenceTime());
    BOOST_CHECK(util1.StatsDuration() == util2.StatsDuration());
}

BOOST_AUTO_TEST_CASE(Test_BandwidthUtilisation_OperatorPlusTest)
{
    BandwidthUtilisationMock util(std::chrono::seconds(1));

    auto newUtil = util + 100;

    BOOST_CHECK(newUtil.History().size() == 1);
    BOOST_CHECK(util.History().size() == 0);
}

BOOST_AUTO_TEST_CASE(Test_BandwidthUtilisation_UpdateBytes)
{
    BandwidthUtilisationMock bandwidth(std::chrono::seconds(1));
    bandwidth += 100;

    BOOST_CHECK_CLOSE(bandwidth.Utilisation(), (100.0 / 960.0) * 100, 0.01f);
}

BOOST_AUTO_TEST_CASE(Test_BandwidthUtilisation_OneSecondUtilization)
{
    auto initial_time = AqualinkAutomate::Test::MockChronoClocks::SteadyClock::now();
    BandwidthUtilisationMock bandwidth(std::chrono::seconds(1));

    AqualinkAutomate::Test::MockChronoClocks::SteadyClock::set_next_time_point(initial_time + std::chrono::milliseconds(250));
    bandwidth += 100;
    
    AqualinkAutomate::Test::MockChronoClocks::SteadyClock::set_next_time_point(initial_time + std::chrono::milliseconds(250));
    bandwidth += 100;
    
    AqualinkAutomate::Test::MockChronoClocks::SteadyClock::set_next_time_point(initial_time + std::chrono::milliseconds(250));
    bandwidth += 100;
    
    AqualinkAutomate::Test::MockChronoClocks::SteadyClock::set_next_time_point(initial_time + std::chrono::milliseconds(250));
    bandwidth += 100;

    BOOST_CHECK_CLOSE(bandwidth.Utilisation(), 41.6667f, 0.01f);

    AqualinkAutomate::Test::MockChronoClocks::SteadyClock::set_next_time_point(initial_time + std::chrono::milliseconds(1250));
    bandwidth += 100;

    AqualinkAutomate::Test::MockChronoClocks::SteadyClock::set_next_time_point(initial_time + std::chrono::milliseconds(1500));
    bandwidth += 100;

    AqualinkAutomate::Test::MockChronoClocks::SteadyClock::set_next_time_point(initial_time + std::chrono::milliseconds(1750));
    bandwidth += 100;

    AqualinkAutomate::Test::MockChronoClocks::SteadyClock::set_next_time_point(initial_time + std::chrono::milliseconds(2000));
    bandwidth += 100;

    BOOST_CHECK_CLOSE(bandwidth.Utilisation(), 41.6667f, 0.01f);
}

BOOST_AUTO_TEST_CASE(Test_BandwidthUtilisation_ThirtySecondsUtilization)
{
    auto initial_time = AqualinkAutomate::Test::MockChronoClocks::SteadyClock::now();
    BandwidthUtilisationMock bandwidth(std::chrono::seconds(30));

    { 
        std::vector<uint64_t> bytes_to_add =
        {
            10, 10,
            100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
            100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
            100, 100, 100, 100, 100, 100, 100, 100, 100, 100
        };

        for (auto bytes : bytes_to_add)
        {
            bandwidth += bytes;
            initial_time += std::chrono::seconds(1);

            AqualinkAutomate::Test::MockChronoClocks::SteadyClock::set_next_time_point(initial_time);
        }
    }

    BOOST_CHECK_CLOSE(bandwidth.Utilisation(), 10.4514f, 0.01f);

    {
        std::vector<uint64_t> bytes_to_add =
        {
            200, 200, 200, 200, 200, 
            200, 200, 200, 200, 200,
            200, 200, 200, 200, 200
        };

        for (auto bytes : bytes_to_add)
        {
            bandwidth += bytes;
            initial_time += std::chrono::seconds(1);

            AqualinkAutomate::Test::MockChronoClocks::SteadyClock::set_next_time_point(initial_time);
        }
    }

    BOOST_CHECK_CLOSE(bandwidth.Utilisation(), 15.9722f, 0.01f);
}

BOOST_AUTO_TEST_CASE(Test_BandwidthUtilisation_FiveMinutesUtilization)
{
    auto initial_time = AqualinkAutomate::Test::MockChronoClocks::SteadyClock::now();
    BandwidthUtilisationMock bandwidth(std::chrono::minutes(5));

    AqualinkAutomate::Test::MockChronoClocks::SteadyClock::set_next_time_point(initial_time + std::chrono::minutes(5));

    bandwidth += 100;

    BOOST_CHECK_CLOSE(bandwidth.Utilisation(), 0.03472f, 0.01f);
}

BOOST_AUTO_TEST_SUITE_END();
