#include <boost/test/unit_test.hpp>

#include "jandy/config/jandy_config_auxillary.h"
#include "jandy/errors/string_conversion_errors.h"
#include "jandy/utility/string_conversion/auxillary_state.h"

#include "support/unit_test_ostream_support.h"

BOOST_AUTO_TEST_SUITE(AuxillaryState);

BOOST_AUTO_TEST_CASE(ConstructorTest)
{
    using AqualinkAutomate::Config::AuxillaryStates::On;
    using AqualinkAutomate::Utility::AuxillaryState;

    AuxillaryState status("Pump  ON");
    BOOST_REQUIRE(status.Label().has_value());
    BOOST_REQUIRE(status.State().has_value());
    BOOST_CHECK_EQUAL(status.Label().value(), "Pump");
    BOOST_CHECK_EQUAL(status.State().value(), AuxillaryStates::On);
}

BOOST_AUTO_TEST_CASE(AssignmentOperatorTest)
{
    using AqualinkAutomate::Config::AuxillaryStates::Off;
    using AqualinkAutomate::Utility::AuxillaryState;

    AuxillaryState status;
    status = "Pump  OFF";
    BOOST_REQUIRE(status.Label().has_value());
    BOOST_REQUIRE(status.State().has_value());
    BOOST_CHECK_EQUAL(status.Label().value(), "Pump");
    BOOST_CHECK_EQUAL(status.State().value(), AuxillaryStates::Off);
}

BOOST_AUTO_TEST_CASE(InvalidStringTest)
{
    using AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
    using AqualinkAutomate::Utility::AuxillaryState;

    AuxillaryState status("Invalid string");
    BOOST_REQUIRE(!status.Label().has_value());
    BOOST_REQUIRE(!status.State().has_value());
    BOOST_CHECK_EQUAL(status.Label().error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
    BOOST_CHECK_EQUAL(status.State().error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
}

BOOST_AUTO_TEST_CASE(IncorrectlyFormattedStringTest)
{
    using AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
    using AqualinkAutomate::Utility::AuxillaryState;

    AuxillaryState status("PumpON");
    BOOST_REQUIRE(!status.Label().has_value());
    BOOST_REQUIRE(!status.State().has_value());
    BOOST_CHECK_EQUAL(status.Label().error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
    BOOST_CHECK_EQUAL(status.State().error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
}

BOOST_AUTO_TEST_CASE(WrongLengthStringTest)
{
    using AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
    using AqualinkAutomate::Utility::AuxillaryState;

    AuxillaryState status("This string is too long");
    BOOST_REQUIRE(!status.Label().has_value());
    BOOST_REQUIRE(!status.State().has_value());
    BOOST_CHECK_EQUAL(status.Label().error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
    BOOST_CHECK_EQUAL(status.State().error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
}

BOOST_AUTO_TEST_CASE(EmptyStringTest)
{
    using AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
    using AqualinkAutomate::Utility::AuxillaryState;

    AuxillaryState status("");
    BOOST_REQUIRE(!status.Label().has_value());
    BOOST_REQUIRE(!status.State().has_value());
    BOOST_CHECK_EQUAL(status.Label().error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
    BOOST_CHECK_EQUAL(status.State().error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
}

BOOST_AUTO_TEST_CASE(StatusTypesTest)
{
    using AqualinkAutomate::Config::AuxillaryStates::On;
    using AqualinkAutomate::Config::AuxillaryStates::Off;
    using AqualinkAutomate::Config::AuxillaryStates::Enabled;
    using AqualinkAutomate::Config::AuxillaryStates::Pending;
    using AqualinkAutomate::Utility::AuxillaryState;

    AuxillaryState status1("Pump  ON");
    BOOST_REQUIRE(status1.State().has_value());
    BOOST_CHECK_EQUAL(status1.State().value(), AuxillaryStates::On);

    AuxillaryState status2("Pump  OFF");
    BOOST_REQUIRE(status2.State().has_value());
    BOOST_CHECK_EQUAL(status2.State().value(), AuxillaryStates::Off);

    AuxillaryState status3("Pump  ENA");
    BOOST_REQUIRE(status3.State().has_value());
    BOOST_CHECK_EQUAL(status3.State().value(), AuxillaryStates::Enabled);

    AuxillaryState status4("Pump  ***");
    BOOST_REQUIRE(status4.State().has_value());
    BOOST_CHECK_EQUAL(status4.State().value(), AuxillaryStates::Pending);
}

BOOST_AUTO_TEST_CASE(MaximumLengthNameTest)
{
    using AqualinkAutomate::Config::AuxillaryStates;
    using AqualinkAutomate::Utility::AuxillaryState;

    AuxillaryState status("A_MaximumName ON");
    BOOST_REQUIRE(status.Label().has_value());
    BOOST_CHECK_EQUAL(status.Label().value(), "A_MaximumName");
}

BOOST_AUTO_TEST_CASE(MinimumLengthNameTest)
{
    using AqualinkAutomate::Utility::AuxillaryState;

    AuxillaryState status("A ON");
    BOOST_REQUIRE(status.Label().has_value());
    BOOST_CHECK_EQUAL(status.Label().value(), "A");
}

BOOST_AUTO_TEST_CASE(NameWithSpacesTest)
{
    using AqualinkAutomate::Utility::AuxillaryState;

    AuxillaryState status("A MaximumName ON");
    BOOST_REQUIRE(status.Label().has_value());
    BOOST_CHECK_EQUAL(status.Label().value(), "A MaximumName");
}

BOOST_AUTO_TEST_SUITE_END();
