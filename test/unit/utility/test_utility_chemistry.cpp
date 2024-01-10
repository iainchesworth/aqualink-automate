#include <sstream>

#include <boost/test/unit_test.hpp>

#include "formatters/orp_formatter.h"
#include "formatters/ph_formatter.h"
#include "jandy/formatters/chemistry_formatter.h"
#include "jandy/utility/string_conversion/chemistry_string_converter.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate::Utility;

BOOST_AUTO_TEST_SUITE(TestSuite_Chemistry);

BOOST_AUTO_TEST_CASE(Test_Chemistry_ValidChemistryStrings)
{
    ChemistryStringConverter chem1("ORP/200 PH/6.8");
    BOOST_REQUIRE(chem1.ORP().has_value());
    BOOST_CHECK_EQUAL(chem1.ORP().value(), 200);
    BOOST_REQUIRE(chem1.PH().has_value());
    BOOST_CHECK_EQUAL(chem1.PH().value(), 6.8f);

    ChemistryStringConverter chem2("ORP/950 PH/8.2");
    BOOST_REQUIRE(chem2.ORP().has_value());
    BOOST_CHECK_EQUAL(chem2.ORP().value(), 950);
    BOOST_REQUIRE(chem2.PH().has_value());
    BOOST_CHECK_EQUAL(chem2.PH().value(), 8.2f);
}

BOOST_AUTO_TEST_CASE(Test_Chemistry_InvalidChemistryStrings)
{
    // Test string length below minimum
    ChemistryStringConverter chem1("ORP/2");
    BOOST_CHECK(!chem1.ORP().has_value());
    BOOST_CHECK(!chem1.PH().has_value());

    // Test string length above maximum
    ChemistryStringConverter chem2("ORP/10000 PH/10.00");
    BOOST_CHECK(!chem1.ORP().has_value());
    BOOST_CHECK(!chem1.PH().has_value());

    // Test with negative values
    ChemistryStringConverter chem3("ORP/-300 PH/6.5");
    BOOST_CHECK(!chem1.ORP().has_value());
    BOOST_CHECK(!chem1.PH().has_value());

    // Test with non-numeric values
    ChemistryStringConverter chem4("ORP/ABC PH/DEF");
    BOOST_CHECK(!chem1.ORP().has_value());
    BOOST_CHECK(!chem1.PH().has_value());

    // Test with missing ORP and PH fields
    ChemistryStringConverter chem5("200 6.8");
    BOOST_CHECK(!chem1.ORP().has_value());
    BOOST_CHECK(!chem1.PH().has_value());
}

BOOST_AUTO_TEST_CASE(Test_Chemistry_CopyAssignmentOperators)
{
    ChemistryStringConverter chem1("ORP/500 PH/7.0");
    ChemistryStringConverter chem2 = chem1; // Copy assignment

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
    ChemistryStringConverter chem1("ORP/600 PH/7.2");
    ChemistryStringConverter chem2 = std::move(chem1); // Move assignment

    BOOST_REQUIRE(chem2.ORP().has_value());
    BOOST_CHECK_EQUAL(chem2.ORP().value(), 600);
    BOOST_REQUIRE(chem2.PH().has_value());
    BOOST_CHECK_EQUAL(chem2.PH().value(), 7.2f);
}

BOOST_AUTO_TEST_CASE(OstreamOperatorNormalCase)
{
    ChemistryStringConverter chem1("ORP/200 PH/6.8");

    std::ostringstream oss;
    oss << chem1;
    std::string result = oss.str();

    BOOST_CHECK_EQUAL(result, "ORP=200mV PH=6.8");
}

BOOST_AUTO_TEST_CASE(OstreamOperatorExceptionCase)
{
    ChemistryStringConverter chem1("ORP/2");

    std::ostringstream oss;
    oss << chem1;
    std::string result = oss.str();

    BOOST_CHECK_EQUAL(result, "CHEM=??");
}

BOOST_AUTO_TEST_CASE(FormatNormalCase)
{
    ChemistryStringConverter chem1("ORP/200 PH/6.8");
    ChemistryStringConverter chem2("ORP/950 PH/8.2");

    {
        std::string result = std::format("{}", chem1);
        BOOST_CHECK_EQUAL(result, "ORP=200mV PH=6.8");
    }

    {
        std::string result = std::format("{}", chem2);
        BOOST_CHECK_EQUAL(result, "ORP=950mV PH=8.2");
    }
}

BOOST_AUTO_TEST_CASE(FormatExceptionCase)
{
    {
        ChemistryStringConverter chem1("ORP/2");
        std::string result = std::format("{}", chem1);
        BOOST_CHECK_EQUAL(result, "CHEM=??");
    }

    {
        ChemistryStringConverter chem2("ORP/10000 PH/10.00");
        std::string result = std::format("{}", chem2);
        BOOST_CHECK_EQUAL(result, "CHEM=??");
    }

    {
        ChemistryStringConverter chem3("ORP/-300 PH/6.5");
        std::string result = std::format("{}", chem3);
        BOOST_CHECK_EQUAL(result, "CHEM=??");
    }

    {
        ChemistryStringConverter chem4("ORP/ABC PH/DEF");
        std::string result = std::format("{}", chem4);
        BOOST_CHECK_EQUAL(result, "CHEM=??");
    }

    {
        ChemistryStringConverter chem5("200 6.8");
        std::string result = std::format("{}", chem5);
        BOOST_CHECK_EQUAL(result, "CHEM=??");
    }
}

BOOST_AUTO_TEST_SUITE_END();
