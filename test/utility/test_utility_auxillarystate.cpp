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
    auto result = status();
    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(result.value().Label, "Pump");
    BOOST_CHECK_EQUAL(result.value().State, AuxillaryStates::On);
}

BOOST_AUTO_TEST_CASE(AssignmentOperatorTest)
{
    using AqualinkAutomate::Config::AuxillaryStates::Off;
    using AqualinkAutomate::Utility::AuxillaryState;

    AuxillaryState status;
    status = "Pump  OFF";
    auto result = status();
    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(result.value().Label, "Pump");
    BOOST_CHECK_EQUAL(result.value().State, AuxillaryStates::Off);
}

BOOST_AUTO_TEST_CASE(InvalidStringTest)
{
    using AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
    using AqualinkAutomate::Utility::AuxillaryState;

    AuxillaryState status("Invalid string");
    auto result = status();
    BOOST_CHECK(!result);
    BOOST_CHECK_EQUAL(result.error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
}

BOOST_AUTO_TEST_CASE(IncorrectlyFormattedStringTest)
{
    using AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
    using AqualinkAutomate::Utility::AuxillaryState;

    AuxillaryState status("PumpON");
    auto result = status();
    BOOST_CHECK(!result);
    BOOST_CHECK_EQUAL(result.error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
}

BOOST_AUTO_TEST_CASE(WrongLengthStringTest)
{
    using AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
    using AqualinkAutomate::Utility::AuxillaryState;

    AuxillaryState status("This string is too long");
    auto result = status();
    BOOST_CHECK(!result);
    BOOST_CHECK_EQUAL(result.error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
}

BOOST_AUTO_TEST_CASE(EmptyStringTest)
{
    using AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
    using AqualinkAutomate::Utility::AuxillaryState;

    AuxillaryState status("");
    auto result = status();
    BOOST_CHECK(!result);
    BOOST_CHECK_EQUAL(result.error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
}

BOOST_AUTO_TEST_CASE(StatusTypesTest)
{
    using AqualinkAutomate::Config::AuxillaryStates::On;
    using AqualinkAutomate::Config::AuxillaryStates::Off;
    using AqualinkAutomate::Config::AuxillaryStates::Enabled;
    using AqualinkAutomate::Config::AuxillaryStates::Pending;
    using AqualinkAutomate::Utility::AuxillaryState;

    AuxillaryState status1("Pump  ON");
    auto result1 = status1.State();
    BOOST_CHECK(result1);
    BOOST_CHECK_EQUAL(result1.value(), AuxillaryStates::On);

    AuxillaryState status2("Pump  OFF");
    auto result2 = status2.State();
    BOOST_CHECK(result2);
    BOOST_CHECK_EQUAL(result2.value(), AuxillaryStates::Off);

    AuxillaryState status3("Pump  ENA");
    auto result3 = status3.State();
    BOOST_CHECK(result3);
    BOOST_CHECK_EQUAL(result3.value(), AuxillaryStates::Enabled);

    AuxillaryState status4("Pump  ***");
    auto result4 = status4.State();
    BOOST_CHECK(result4);
    BOOST_CHECK_EQUAL(result4.value(), AuxillaryStates::Pending);
}

BOOST_AUTO_TEST_CASE(MaximumLengthNameTest)
{
    using AqualinkAutomate::Config::AuxillaryStates;
    using AqualinkAutomate::Utility::AuxillaryState;

    AuxillaryState status("A_MaximumName ON");
    auto result = status.Label();
    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(result.value(), "A_MaximumName");
}

BOOST_AUTO_TEST_CASE(MinimumLengthNameTest)
{
    using AqualinkAutomate::Utility::AuxillaryState;

    AuxillaryState status("A ON");
    auto result = status.Label();
    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(result.value(), "A");
}

BOOST_AUTO_TEST_CASE(NameWithSpacesTest)
{
    using AqualinkAutomate::Utility::AuxillaryState;

    AuxillaryState status("A MaximumName ON");
    auto result = status.Label();
    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(result.value(), "A MaximumName");
}

BOOST_AUTO_TEST_SUITE_END();
