#include <boost/test/unit_test.hpp>

#include "jandy/errors/string_conversion_errors.h"
#include "jandy/utility/string_conversion/equipment_status.h"

#include "support/unit_test_ostream_support.h"

BOOST_AUTO_TEST_SUITE(EquipmentStatus);

BOOST_AUTO_TEST_CASE(ConstructorTest)
{
    using AqualinkAutomate::Utility::EquipmentStatus;

    EquipmentStatus status("Pump  ON");
    auto result = status();
    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(std::get<0>(result.value()), "Pump");
    BOOST_CHECK_EQUAL(std::get<1>(result.value()), EquipmentStatus::Statuses::On);
}

BOOST_AUTO_TEST_CASE(AssignmentOperatorTest)
{
    using AqualinkAutomate::Utility::EquipmentStatus;

    EquipmentStatus status;
    status = "Pump  OFF";
    auto result = status();
    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(std::get<0>(result.value()), "Pump");
    BOOST_CHECK_EQUAL(std::get<1>(result.value()), EquipmentStatus::Statuses::Off);
}

BOOST_AUTO_TEST_CASE(InvalidStringTest)
{
    using AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
    using AqualinkAutomate::Utility::EquipmentStatus;

    EquipmentStatus status("Invalid string");
    auto result = status();
    BOOST_CHECK(!result);
    BOOST_CHECK_EQUAL(result.error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
}

BOOST_AUTO_TEST_CASE(IncorrectlyFormattedStringTest)
{
    using AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
    using AqualinkAutomate::Utility::EquipmentStatus;

    EquipmentStatus status("PumpON");
    auto result = status();
    BOOST_CHECK(!result);
    BOOST_CHECK_EQUAL(result.error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
}

BOOST_AUTO_TEST_CASE(WrongLengthStringTest)
{
    using AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
    using AqualinkAutomate::Utility::EquipmentStatus;

    EquipmentStatus status("This string is too long");
    auto result = status();
    BOOST_CHECK(!result);
    BOOST_CHECK_EQUAL(result.error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
}

BOOST_AUTO_TEST_CASE(EmptyStringTest)
{
    using AqualinkAutomate::ErrorCodes::StringConversion_ErrorCodes::MalformedInput;
    using AqualinkAutomate::Utility::EquipmentStatus;

    EquipmentStatus status("");
    auto result = status();
    BOOST_CHECK(!result);
    BOOST_CHECK_EQUAL(result.error(), make_error_code(StringConversion_ErrorCodes::MalformedInput));
}

BOOST_AUTO_TEST_CASE(StatusTypesTest)
{
    using AqualinkAutomate::Utility::EquipmentStatus;

    EquipmentStatus status1("Pump  ON");
    auto result1 = status1.Status();
    BOOST_CHECK(result1);
    BOOST_CHECK_EQUAL(result1.value(), EquipmentStatus::Statuses::On);

    EquipmentStatus status2("Pump  OFF");
    auto result2 = status2.Status();
    BOOST_CHECK(result2);
    BOOST_CHECK_EQUAL(result2.value(), EquipmentStatus::Statuses::Off);

    EquipmentStatus status3("Pump  ENA");
    auto result3 = status3.Status();
    BOOST_CHECK(result3);
    BOOST_CHECK_EQUAL(result3.value(), EquipmentStatus::Statuses::Enabled);

    EquipmentStatus status4("Pump  ***");
    auto result4 = status4.Status();
    BOOST_CHECK(result4);
    BOOST_CHECK_EQUAL(result4.value(), EquipmentStatus::Statuses::Pending);
}

BOOST_AUTO_TEST_CASE(MaximumLengthNameTest)
{
    using AqualinkAutomate::Utility::EquipmentStatus;

    EquipmentStatus status("A_MaximumName ON");
    auto result = status.Name();
    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(result.value(), "A_MaximumName");
}

BOOST_AUTO_TEST_CASE(MinimumLengthNameTest)
{
    using AqualinkAutomate::Utility::EquipmentStatus;

    EquipmentStatus status("A ON");
    auto result = status.Name();
    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(result.value(), "A");
}

BOOST_AUTO_TEST_CASE(NameWithSpacesTest)
{
    using AqualinkAutomate::Utility::EquipmentStatus;

    EquipmentStatus status("A MaximumName ON");
    auto result = status.Name();
    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(result.value(), "A MaximumName");
}

BOOST_AUTO_TEST_SUITE_END();
