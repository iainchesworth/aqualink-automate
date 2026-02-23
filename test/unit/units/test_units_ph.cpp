#include <boost/test/unit_test.hpp>

#include "formatters/ph_formatter.h"
#include "kernel/ph.h"

BOOST_AUTO_TEST_SUITE(PHTestSuite)

BOOST_AUTO_TEST_CASE(ConstructorTest)
{
    using AqualinkAutomate::Kernel::pH;

    pH pHValue1(-1.5f);
    BOOST_CHECK_EQUAL(pHValue1(), 0.0f);

    pH pHValue2(7.566f);
    BOOST_CHECK_EQUAL(pHValue2(), 7.6f);

    pH pHValue3(15.2f);
    BOOST_CHECK_EQUAL(pHValue3(), 14.0f);
}

BOOST_AUTO_TEST_CASE(AssignmentOperatorTest)
{
    using AqualinkAutomate::Kernel::pH;

    pH pHValue1(0.0f);
    pHValue1 = -2.4f;
    BOOST_CHECK_EQUAL(pHValue1(), 0.0f);

    pH pHValue2(0.0f);
    pHValue2 = 6.821f;
    BOOST_CHECK_EQUAL(pHValue2(), 6.8f);

    pH pHValue3(0.0f);
    pHValue3 = 19.3f;
    BOOST_CHECK_EQUAL(pHValue3(), 14.0f);
}

BOOST_AUTO_TEST_CASE(OstreamOperatorNormalCase)
{
    {
        AqualinkAutomate::Kernel::pH pH_value(7.35f);

        std::ostringstream oss;
        oss << pH_value;
        std::string result = oss.str();

        BOOST_CHECK_EQUAL(result, "7.4");
    }
    {
        AqualinkAutomate::Kernel::pH pH_value(8.f);

        std::ostringstream oss;
        oss << pH_value;
        std::string result = oss.str();

        BOOST_CHECK_EQUAL(result, "8.0");
    }
}

BOOST_AUTO_TEST_CASE(FormatNormalCase)
{
    {
        AqualinkAutomate::Kernel::pH pH_value(7.35f);
        std::string result = std::format("{}", pH_value);
        BOOST_CHECK_EQUAL(result, "7.4");
    }

    {
        AqualinkAutomate::Kernel::pH pH_value(8.f);
        std::string result = std::format("{}", pH_value);
        BOOST_CHECK_EQUAL(result, "8.0");
    }
}

BOOST_AUTO_TEST_SUITE_END()
