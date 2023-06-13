#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_ids.h"

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
    BOOST_CHECK(message.AckType() == AckTypes::Unknown);
    BOOST_CHECK(message.Command() == 0x00);
}

BOOST_AUTO_TEST_CASE(TestJandyMessage_AckCustomConstruction)
{
    JandyMessage_Ack message(AckTypes::ACK_IAQTouch, 0x10);
    BOOST_CHECK(message.AckType() == AckTypes::ACK_IAQTouch);
    BOOST_CHECK(message.Command() == 0x10);
}

BOOST_AUTO_TEST_CASE(TestSerializationDeserialization)
{
    JandyMessage_Ack message1(AckTypes::ACK_IAQTouch, 0x10);
    std::vector<uint8_t> serializedMessage;
    BOOST_TEST_REQUIRE(message1.Serialize(serializedMessage));

    JandyMessage_Ack message2;
    BOOST_TEST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(serializedMessage))));

    BOOST_CHECK(message1.AckType() == message2.AckType());
    BOOST_CHECK(message1.Command() == message2.Command());
}

BOOST_AUTO_TEST_SUITE_END()

