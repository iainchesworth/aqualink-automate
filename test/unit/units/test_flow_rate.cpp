#include <boost/test/unit_test.hpp>

#include "kernel/flow_rate.h"

using namespace AqualinkAutomate::Kernel;
using namespace AqualinkAutomate::Units;

BOOST_AUTO_TEST_SUITE(FlowRateTestSuite, *boost::unit_test::tolerance(0.00001))

BOOST_AUTO_TEST_CASE(ConstructorTest)
{
    FlowRate flowRateGpm(10.0f * gpm);
    FlowRate flowRateLpm(37.8541f * lpm);

    BOOST_TEST(flowRateGpm.InGPM().value() == 10.0f);
    BOOST_TEST(flowRateLpm.InLPM().value() == 37.8541f);
}

BOOST_AUTO_TEST_CASE(ConvertToFlowRateInGPMTest)
{
    FlowRate flowRate = FlowRate::ConvertToFlowRateInGPM(1.0f);

    BOOST_TEST(flowRate.InGPM().value() == 1.0f);
    BOOST_TEST(flowRate.InLPM().value() == 3.785412f);
}

BOOST_AUTO_TEST_CASE(ConvertToFlowRateInLPMTest)
{
    FlowRate flowRate = FlowRate::ConvertToFlowRateInLPM(1.0f);

    BOOST_TEST(flowRate.InLPM().value() == 1.0f);
    BOOST_TEST(flowRate.InGPM().value() == 0.264172f);
}

BOOST_AUTO_TEST_CASE(InGPMTest)
{
    FlowRate flowRate(5 * gpm);

    BOOST_TEST(flowRate.InGPM().value() == 5);
}

BOOST_AUTO_TEST_CASE(InLPMTest)
{
    FlowRate flowRate(5 * lpm);

    BOOST_TEST(flowRate.InLPM().value() == 5);
}

BOOST_AUTO_TEST_SUITE_END()
