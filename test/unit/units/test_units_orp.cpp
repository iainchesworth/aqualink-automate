#include <boost/test/unit_test.hpp>

#include "formatters/orp_formatter.h"
#include "kernel/orp.h"

BOOST_AUTO_TEST_SUITE(ORPTestSuite)

BOOST_AUTO_TEST_CASE(ConstructorTest)
{
    using AqualinkAutomate::Kernel::ORP;
    using AqualinkAutomate::Units::millivolt;
    using AqualinkAutomate::Units::millivolts;

    ORP orp(500.0);
    BOOST_CHECK_EQUAL(orp(), 500.0 * millivolt);
}

BOOST_AUTO_TEST_CASE(AssignmentOperatorTest)
{
    using AqualinkAutomate::Kernel::ORP;
    using AqualinkAutomate::Units::millivolt;
    using AqualinkAutomate::Units::millivolts;

    ORP orp(0.0);
    orp = 800.0;
    BOOST_CHECK_EQUAL(orp(), 800.0 * millivolt);
}

BOOST_AUTO_TEST_SUITE_END()
