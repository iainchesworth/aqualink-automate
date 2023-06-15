#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/formatters/jandy_device_formatters.h"
#include "jandy/formatters/jandy_message_formatters.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_ids.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Messages;

BOOST_AUTO_TEST_SUITE(JandyMessage_AckTestSuite)

BOOST_AUTO_TEST_CASE(TestAckTypes)
{
    BOOST_CHECK(static_cast<uint8_t>(AckTypes::ACK_IAQTouch) == 0x00);
    BOOST_CHECK(static_cast<uint8_t>(AckTypes::ACK_PDA) == 0x40);
    // add similar cases for other enum values
}

BOOST_AUTO_TEST_CASE(TestJandyMessage_AckConstruction)
{
    JandyMessage_Ack message;

    BOOST_CHECK_EQUAL(message.Destination(), JandyDeviceId(0x00));
    BOOST_CHECK_EQUAL(message.Id(), JandyMessageIds::Ack);
    BOOST_CHECK_EQUAL(message.RawId(), 0x0);
    BOOST_CHECK_EQUAL(message.MessageLength(), 0);
    BOOST_CHECK_EQUAL(message.ChecksumValue(), 0);

    BOOST_CHECK_EQUAL(message.AckType(), AckTypes::Unknown);
    BOOST_CHECK_EQUAL(message.Command(), 0x00);
}

BOOST_AUTO_TEST_CASE(TestJandyMessage_AckCustomConstruction)
{
    JandyMessage_Ack message(AckTypes::ACK_IAQTouch, 0x10);

    BOOST_CHECK_EQUAL(message.Destination(), JandyDeviceId(0x00));
    BOOST_CHECK_EQUAL(message.Id(), JandyMessageIds::Ack);
    BOOST_CHECK_EQUAL(message.RawId(), 0x0);
    BOOST_CHECK_EQUAL(message.MessageLength(), 0);
    BOOST_CHECK_EQUAL(message.ChecksumValue(), 0);

    BOOST_CHECK_EQUAL(message.AckType(), AckTypes::ACK_IAQTouch);
    BOOST_CHECK_EQUAL(message.Command(), 0x10);
}

BOOST_AUTO_TEST_CASE(TestSerializationDeserialization)
{
    JandyMessage_Ack message1(AckTypes::ACK_IAQTouch, 0x10);
    JandyMessage_Ack message2;

    std::vector<uint8_t> serializedMessage;
    BOOST_REQUIRE(message1.Serialize(serializedMessage));
    BOOST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(serializedMessage))));

    BOOST_CHECK_EQUAL(message1.Destination(), message2.Destination());
    BOOST_CHECK_EQUAL(message1.Id(), message2.Id());
    BOOST_CHECK_NE(message1.RawId(), message2.RawId()); // Deserialisation captures the message "raw" id...
    BOOST_CHECK_EQUAL(0x01, message2.RawId());
    BOOST_CHECK_NE(message1.MessageLength(), message2.MessageLength());  // Deserialisation captures the message length...
    BOOST_CHECK_EQUAL(10, message2.MessageLength());
    BOOST_CHECK_NE(message1.ChecksumValue(), message2.ChecksumValue());  // Deserialisation captures the message checksum value...
    BOOST_CHECK_EQUAL(0x10, message2.ChecksumValue());

    BOOST_CHECK(message1.AckType() == message2.AckType());
    BOOST_CHECK(message1.Command() == message2.Command());
}

BOOST_AUTO_TEST_SUITE_END()

