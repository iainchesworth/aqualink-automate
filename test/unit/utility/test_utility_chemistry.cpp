#include <boost/test/unit_test.hpp>

#include "jandy/formatters/chemistry_formatter.h"
#include "jandy/utility/string_conversion/chemistry.h"

#include "support/unit_test_ostream_support.h"

BOOST_AUTO_TEST_SUITE(TestSuite_Chemistry);

BOOST_AUTO_TEST_CASE(Test_Chemistry_ValidChemistryStrings)
{
    AqualinkAutomate::Utility::Chemistry chem1("ORP/200 PH/6.8");
    BOOST_REQUIRE(chem1.ORP().has_value());
    BOOST_CHECK_EQUAL(chem1.ORP().value(), 200);
    BOOST_REQUIRE(chem1.PH().has_value());
    BOOST_CHECK_EQUAL(chem1.PH().value(), 6.8f);

    AqualinkAutomate::Utility::Chemistry chem2("ORP/950 PH/8.2");
    BOOST_REQUIRE(chem2.ORP().has_value());
    BOOST_CHECK_EQUAL(chem2.ORP().value(), 950);
    BOOST_REQUIRE(chem2.PH().has_value());
    BOOST_CHECK_EQUAL(chem2.PH().value(), 8.2f);
}

BOOST_AUTO_TEST_CASE(Test_Chemistry_InvalidChemistryStrings)
{
    // Test string length below minimum
    AqualinkAutomate::Utility::Chemistry chem1("ORP/2");
    BOOST_CHECK(!chem1.ORP().has_value());
    BOOST_CHECK(!chem1.PH().has_value());

    // Test string length above maximum
    AqualinkAutomate::Utility::Chemistry chem2("ORP/10000 PH/10.00");
    BOOST_CHECK(!chem1.ORP().has_value());
    BOOST_CHECK(!chem1.PH().has_value());

    // Test with negative values
    AqualinkAutomate::Utility::Chemistry chem3("ORP/-300 PH/6.5");
    BOOST_CHECK(!chem1.ORP().has_value());
    BOOST_CHECK(!chem1.PH().has_value());

    // Test with non-numeric values
    AqualinkAutomate::Utility::Chemistry chem4("ORP/ABC PH/DEF");
    BOOST_CHECK(!chem1.ORP().has_value());
    BOOST_CHECK(!chem1.PH().has_value());

    // Test with missing ORP and PH fields
    AqualinkAutomate::Utility::Chemistry chem5("200 6.8");
    BOOST_CHECK(!chem1.ORP().has_value());
    BOOST_CHECK(!chem1.PH().has_value());
}

BOOST_AUTO_TEST_CASE(Test_Chemistry_CopyAssignmentOperators)
{
    AqualinkAutomate::Utility::Chemistry chem1("ORP/500 PH/7.0");
    AqualinkAutomate::Utility::Chemistry chem2 = chem1; // Copy assignment

    BOOST_REQUIRE(chem1.ORP().has_value());
    BOOST_CHECK_EQUAL(chem1.ORP().value(), 500);
    BOOST_REQUIRE(chem1.PH().has_value());
    BOOST_CHECK_EQUAL(chem1.PH().value(), 7.0f);

    BOOST_REQUIRE(chem2.ORP().has_value());
    BOOST_CHECK_EQUAL(chem2.ORP().value(), 500);
    BOOST_REQUIRE(chem2.PH().has_value());
    BOOST_CHECK_EQUAL(chem2.PH().value(), 7.0f);
}

BOOST_AUTO_TEST_CASE(Test_Chemistry_MoveAssignmentOperator)
{
    AqualinkAutomate::Utility::Chemistry chem1("ORP/600 PH/7.2");
    AqualinkAutomate::Utility::Chemistry chem2 = std::move(chem1); // Move assignment

    BOOST_REQUIRE(chem2.ORP().has_value());
    BOOST_CHECK_EQUAL(chem2.ORP().value(), 600);
    BOOST_REQUIRE(chem2.PH().has_value());
    BOOST_CHECK_EQUAL(chem2.PH().value(), 7.2f);
}

BOOST_AUTO_TEST_SUITE_END();
