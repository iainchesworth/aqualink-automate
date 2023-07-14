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

BOOST_AUTO_TEST_SUITE_END()
