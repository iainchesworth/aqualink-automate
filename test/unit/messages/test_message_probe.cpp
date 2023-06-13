#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_probe.h"

using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Messages;

BOOST_AUTO_TEST_SUITE(JandyMessage_ProbeTestSuite)

BOOST_AUTO_TEST_CASE(TestJandyMessage_ProbeConstruction)
{
    JandyMessage_Probe message;
    BOOST_CHECK(message.Destination() == JandyDeviceId(0xFF));
    BOOST_CHECK(message.RawId() == 0);
    BOOST_CHECK(message.MessageLength() == 0);
    BOOST_CHECK(message.ChecksumValue() == 0);
}

BOOST_AUTO_TEST_CASE(TestSerializationDeserialization)
{
    JandyMessage_Probe message1;
    std::vector<uint8_t> serializedMessage;
    BOOST_TEST_REQUIRE(message1.Serialize(serializedMessage));

    JandyMessage_Probe message2;
    BOOST_TEST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(serializedMessage))));

    BOOST_CHECK(message1.Destination() == message2.Destination());
    BOOST_CHECK(message1.RawId() == message2.RawId());
    BOOST_CHECK(message1.MessageLength() == message2.MessageLength());
    BOOST_CHECK(message1.ChecksumValue() == message2.ChecksumValue());
}

BOOST_AUTO_TEST_CASE(TestToString)
{
    JandyMessage_Probe message;
    std::string expected = "Packet: Destination: AqualinkMaster (0x00), Message Type: Probe (0x00) || Payload: 0";

    BOOST_CHECK_EQUAL(message.ToString(), expected);
}


BOOST_AUTO_TEST_SUITE_END()
