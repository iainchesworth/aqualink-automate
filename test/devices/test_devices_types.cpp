#include <boost/test/unit_test.hpp>

#include "jandy/devices/jandy_device_types.h"

#include "support/unit_test_ostream_support.h"

BOOST_AUTO_TEST_SUITE(JandyDeviceTypeTestSuite)

BOOST_AUTO_TEST_CASE(ConstructorTest)
{
    // Test the default constructor
    AqualinkAutomate::Devices::JandyDeviceType device1;
    BOOST_CHECK_EQUAL(device1.Class(), AqualinkAutomate::Devices::DeviceClasses::Unknown);
    BOOST_CHECK_EQUAL(device1.Id(), 0xFF);

    // Test the constructor that takes a DeviceId
    AqualinkAutomate::Devices::JandyDeviceType device2(0x09);
    BOOST_CHECK_EQUAL(device2.Class(), AqualinkAutomate::Devices::DeviceClasses::RS_Keypad);
    BOOST_CHECK_EQUAL(device2.Id(), 0x09);
}

BOOST_AUTO_TEST_CASE(CopyConstructorTest)
{
    // Initialize a device
    AqualinkAutomate::Devices::JandyDeviceType device1(0x03);

    // Copy construct a device
    AqualinkAutomate::Devices::JandyDeviceType device2(device1);

    BOOST_CHECK_EQUAL(device2.Class(), AqualinkAutomate::Devices::DeviceClasses::AqualinkMaster);
    BOOST_CHECK_EQUAL(device2.Id(), 0x03);
}

BOOST_AUTO_TEST_CASE(MoveConstructorTest)
{
    // Initialize a device
    AqualinkAutomate::Devices::JandyDeviceType device1(0x09);

    // Move construct a device
    AqualinkAutomate::Devices::JandyDeviceType device2(std::move(device1));

    BOOST_CHECK_EQUAL(device2.Class(), AqualinkAutomate::Devices::DeviceClasses::RS_Keypad);
    BOOST_CHECK_EQUAL(device2.Id(), 0x09);
}

BOOST_AUTO_TEST_CASE(AssignmentOperatorTest)
{
    // Initialize two devices
    AqualinkAutomate::Devices::JandyDeviceType device1(0x03);
    AqualinkAutomate::Devices::JandyDeviceType device2(0x09);

    // Test the assignment operator
    device2 = device1;

    BOOST_CHECK_EQUAL(device2.Class(), AqualinkAutomate::Devices::DeviceClasses::AqualinkMaster);
    BOOST_CHECK_EQUAL(device2.Id(), 0x03);
}

BOOST_AUTO_TEST_CASE(MoveAssignmentOperatorTest)
{
    // Initialize two devices
    AqualinkAutomate::Devices::JandyDeviceType device1(0x03);
    AqualinkAutomate::Devices::JandyDeviceType device2(0x09);

    // Test the move assignment operator
    device2 = std::move(device1);

    BOOST_CHECK_EQUAL(device2.Class(), AqualinkAutomate::Devices::DeviceClasses::AqualinkMaster);
    BOOST_CHECK_EQUAL(device2.Id(), 0x03);
}

BOOST_AUTO_TEST_CASE(EqualityOperatorTest)
{
    // Initialize two identical devices
    AqualinkAutomate::Devices::JandyDeviceType device1(0x09);
    AqualinkAutomate::Devices::JandyDeviceType device2(0x09);

    // Test the equality operator
    BOOST_CHECK(device1 == device2);
}

BOOST_AUTO_TEST_CASE(InequalityOperatorTest)
{
    // Initialize two different devices
    AqualinkAutomate::Devices::JandyDeviceType device1(0x03);
    AqualinkAutomate::Devices::JandyDeviceType device2(0x09);

    // Test the inequality operator
    BOOST_CHECK(device1 != device2);
}

BOOST_AUTO_TEST_SUITE_END()
