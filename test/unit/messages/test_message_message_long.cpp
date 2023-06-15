#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/formatters/jandy_device_formatters.h"
#include "jandy/formatters/jandy_message_formatters.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_message_long.h"

using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Messages;

BOOST_AUTO_TEST_SUITE(JandyMessage_MessageLongTestSuite)

BOOST_AUTO_TEST_CASE(TestJandyMessage_MessageLongConstruction)
{
    JandyMessage_MessageLong message;
    BOOST_CHECK_EQUAL(message.Destination(), JandyDeviceId(0x00));
    BOOST_CHECK_EQUAL(message.Id(), JandyMessageIds::MessageLong);
    BOOST_CHECK_EQUAL(message.RawId(), 0x0);
    BOOST_CHECK_EQUAL(message.MessageLength(), 0);
    BOOST_CHECK_EQUAL(message.ChecksumValue(), 0);

    BOOST_CHECK_EQUAL(message.LineId(), 0);
    BOOST_CHECK_EQUAL(message.Line(), "");
}

BOOST_AUTO_TEST_CASE(TestSerializationDeserialization)
{
    const std::string TEST_LINE {"TEST DATA GOES HERE"};
    const uint8_t TEST_LINE_ID{ 5 };

    JandyMessage_MessageLong message1(TEST_LINE_ID, TEST_LINE);
    JandyMessage_MessageLong message2;
    
    std::vector<uint8_t> serializedMessage;
    BOOST_REQUIRE(message1.Serialize(serializedMessage));
    BOOST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(serializedMessage))));

    BOOST_CHECK_EQUAL(message1.Destination(), message2.Destination());
    BOOST_CHECK_EQUAL(message1.Id(), message2.Id());
    BOOST_CHECK_NE(message1.RawId(), message2.RawId()); // Deserialisation captures the message "raw" id...
    BOOST_CHECK_EQUAL(0x04, message2.RawId());
    BOOST_CHECK_NE(message1.MessageLength(), message2.MessageLength());  // Deserialisation captures the message length...
    BOOST_CHECK_EQUAL(27, message2.MessageLength());
    BOOST_CHECK_NE(message1.ChecksumValue(), message2.ChecksumValue());  // Deserialisation captures the message checksum value...
    BOOST_CHECK_EQUAL(0x27, message2.ChecksumValue());

    BOOST_CHECK_EQUAL(TEST_LINE, message2.Line());
    BOOST_CHECK_EQUAL(TEST_LINE_ID, message2.LineId());
}

BOOST_AUTO_TEST_CASE(TestToString)
{
    JandyMessage_MessageLong message;

    const std::string expected = "Packet: Destination: AqualinkMaster (0x00), Message Type: MessageLong (0x04) || Payload: 0";

    BOOST_CHECK_EQUAL(message.ToString(), expected);
}

BOOST_AUTO_TEST_SUITE_END()
