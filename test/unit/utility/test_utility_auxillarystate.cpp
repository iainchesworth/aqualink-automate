#include <boost/test/unit_test.hpp>

#include "jandy/errors/string_conversion_errors.h"
#include "jandy/utility/string_conversion/auxillary_state_string_converter.h"
#include "kernel/auxillary_devices/auxillary_status.h"

#include "support/unit_test_ostream_support.h"

BOOST_AUTO_TEST_SUITE(TestSuite_AuxillaryState);

BOOST_AUTO_TEST_CASE(Test_AuxillaryState_ConstructorTest)
{
    using AqualinkAutomate::Kernel::AuxillaryStatuses::On;
    using AqualinkAutomate::Utility::AuxillaryStateStringConverter;

    AuxillaryStateStringConverter status("Pump  ON");
    BOOST_REQUIRE(status.Label().has_value());
    BOOST_REQUIRE(status.State().has_value());
    BOOST_CHECK_EQUAL(status.Label().value(), "Pump");
    BOOST_CHECK_EQUAL(status.State().value(), AuxillaryStatuses::On);
}

BOOST_AUTO_TEST_CASE(Test_AuxillaryState_AssignmentOperatorTest)
{
    using AqualinkAutomate::Kernel::AuxillaryStatuses::Off;
    using AqualinkAutomate::Utility::AuxillaryStateStringConverter;

    AuxillaryStateStringConverter status;
    status = "Pump  OFF";
    BOOST_REQUIRE(status.Label().has_value());
    BOOST_REQUIRE(status.State().has_value());
    BOOST_CHECK_EQUAL(status.Label().value(), "Pump");
    BOOST_CHECK_EQUAL(status.State().value(), AuxillaryStatuses::Off);
}

BOOST_AUTO_TEST_CASE(Test_AuxillaryState_InvalidStringTest)
{
    using AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
    using AqualinkAutomate::Utility::AuxillaryStateStringConverter;

    AuxillaryStateStringConverter status("Invalid string");
    BOOST_REQUIRE(!status.Label().has_value());
    BOOST_REQUIRE(!status.State().has_value());
    BOOST_CHECK_EQUAL(status.Label().error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
    BOOST_CHECK_EQUAL(status.State().error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
}

BOOST_AUTO_TEST_CASE(Test_AuxillaryState_IncorrectlyFormattedStringTest)
{
    using AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
    using AqualinkAutomate::Utility::AuxillaryStateStringConverter;

    AuxillaryStateStringConverter status("PumpON");
    BOOST_REQUIRE(!status.Label().has_value());
    BOOST_REQUIRE(!status.State().has_value());
    BOOST_CHECK_EQUAL(status.Label().error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
    BOOST_CHECK_EQUAL(status.State().error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
}

BOOST_AUTO_TEST_CASE(Test_AuxillaryState_WrongLengthStringTest)
{
    using AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
    using AqualinkAutomate::Utility::AuxillaryStateStringConverter;

    AuxillaryStateStringConverter status("This string is too long");
    BOOST_REQUIRE(!status.Label().has_value());
    BOOST_REQUIRE(!status.State().has_value());
    BOOST_CHECK_EQUAL(status.Label().error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
    BOOST_CHECK_EQUAL(status.State().error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
}

BOOST_AUTO_TEST_CASE(Test_AuxillaryState_EmptyStringTest)
{
    using AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
    using AqualinkAutomate::Utility::AuxillaryStateStringConverter;

    AuxillaryStateStringConverter status("");
    BOOST_REQUIRE(!status.Label().has_value());
    BOOST_REQUIRE(!status.State().has_value());
    BOOST_CHECK_EQUAL(status.Label().error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
    BOOST_CHECK_EQUAL(status.State().error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
}

BOOST_AUTO_TEST_CASE(Test_AuxillaryState_StatusTypesTest)
{
    using AqualinkAutomate::Kernel::AuxillaryStatuses::On;
    using AqualinkAutomate::Kernel::AuxillaryStatuses::Off;
    using AqualinkAutomate::Kernel::AuxillaryStatuses::Enabled;
    using AqualinkAutomate::Kernel::AuxillaryStatuses::Pending;
    using AqualinkAutomate::Utility::AuxillaryStateStringConverter;

    AuxillaryStateStringConverter status1("Pump  ON");
    BOOST_REQUIRE(status1.State().has_value());
    BOOST_CHECK_EQUAL(status1.State().value(), AuxillaryStatuses::On);

    AuxillaryStateStringConverter status2("Pump  OFF");
    BOOST_REQUIRE(status2.State().has_value());
    BOOST_CHECK_EQUAL(status2.State().value(), AuxillaryStatuses::Off);

    AuxillaryStateStringConverter status3("Pump  ENA");
    BOOST_REQUIRE(status3.State().has_value());
    BOOST_CHECK_EQUAL(status3.State().value(), AuxillaryStatuses::Enabled);

    AuxillaryStateStringConverter status4("Pump  ***");
    BOOST_REQUIRE(status4.State().has_value());
    BOOST_CHECK_EQUAL(status4.State().value(), AuxillaryStatuses::Pending);
}

BOOST_AUTO_TEST_CASE(Test_AuxillaryState_MaximumLengthNameTest)
{
    using AqualinkAutomate::Kernel::AuxillaryStatuses;
    using AqualinkAutomate::Utility::AuxillaryStateStringConverter;

    AuxillaryStateStringConverter status("A_MaximumName ON");
    BOOST_REQUIRE(status.Label().has_value());
    BOOST_CHECK_EQUAL(status.Label().value(), "A_MaximumName");
}

BOOST_AUTO_TEST_CASE(Test_AuxillaryState_MinimumLengthNameTest)
{
    using AqualinkAutomate::Utility::AuxillaryStateStringConverter;

    AuxillaryStateStringConverter status("A ON");
    BOOST_REQUIRE(status.Label().has_value());
    BOOST_CHECK_EQUAL(status.Label().value(), "A");
}

BOOST_AUTO_TEST_CASE(Test_AuxillaryState_NameWithSpacesTest)
{
    using AqualinkAutomate::Utility::AuxillaryStateStringConverter;

    AuxillaryStateStringConverter status("A MaximumName ON");
    BOOST_REQUIRE(status.Label().has_value());
    BOOST_CHECK_EQUAL(status.Label().value(), "A MaximumName");
}

BOOST_AUTO_TEST_SUITE_END();
