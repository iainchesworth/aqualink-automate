#include <boost/test/unit_test.hpp>

#include "jandy/devices/jandy_device_id.h"

#include "support/unit_test_ostream_support.h"

BOOST_AUTO_TEST_SUITE(JandyDeviceIdTestSuite)

BOOST_AUTO_TEST_CASE(ConstructorTest)
{
    // Test the constructor that takes a value_type
    AqualinkAutomate::Devices::JandyDeviceId deviceId(0x09);
    BOOST_CHECK_EQUAL(deviceId(), 0x09);
}

BOOST_AUTO_TEST_CASE(EqualityOperatorTest)
{
    // Initialize two identical device ids
    AqualinkAutomate::Devices::JandyDeviceId deviceId1(0x09);
    AqualinkAutomate::Devices::JandyDeviceId deviceId2(0x09);

    // Test the equality operator
    BOOST_CHECK(deviceId1 == deviceId2);
}

BOOST_AUTO_TEST_CASE(InequalityOperatorTest)
{
    // Initialize two different device ids
    AqualinkAutomate::Devices::JandyDeviceId deviceId1(0x03);
    AqualinkAutomate::Devices::JandyDeviceId deviceId2(0x09);

    // Test the inequality operator
    BOOST_CHECK(deviceId1 != deviceId2);
}

BOOST_AUTO_TEST_CASE(HashTest)
{
    // Initialize a device id
    AqualinkAutomate::Devices::JandyDeviceId deviceId(0x09);

    // Calculate the hash
    std::size_t hash = std::hash<uint8_t>{}(0x09);

    // Test the custom hash function
    BOOST_CHECK_EQUAL(std::hash<AqualinkAutomate::Devices::JandyDeviceId>{}(deviceId), hash);
}

BOOST_AUTO_TEST_SUITE_END()
