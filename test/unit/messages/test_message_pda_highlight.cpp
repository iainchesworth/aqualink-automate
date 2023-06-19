#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/formatters/jandy_device_formatters.h"
#include "jandy/formatters/jandy_message_formatters.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/pda/pda_message_highlight.h"

using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Messages;

BOOST_AUTO_TEST_SUITE(PDAMessage_HighlightTestSuite)

BOOST_AUTO_TEST_CASE(TestPDAMessage_HighlightConstruction)
{
    PDAMessage_Highlight message;
    BOOST_CHECK_EQUAL(message.Destination(), JandyDeviceId(0x00));
    BOOST_CHECK_EQUAL(message.Id(), JandyMessageIds::PDA_Highlight);
    BOOST_CHECK_EQUAL(message.RawId(), 0x0);
    BOOST_CHECK_EQUAL(message.MessageLength(), 0);
    BOOST_CHECK_EQUAL(message.ChecksumValue(), 0);
}

BOOST_AUTO_TEST_CASE(TestSerialization)
{
    PDAMessage_Highlight message;

    std::vector<uint8_t> serialized_message {0x10, 0x02, 0x42, 0x08, 0x0a, 0x00, 0x00, 0x66, 0x10, 0x03};

    BOOST_REQUIRE(message.Deserialize(std::as_bytes(std::span<uint8_t>(serialized_message))));
    BOOST_CHECK_EQUAL(10, message.LineId());
}

BOOST_AUTO_TEST_CASE(TestSerializationDeserialization)
{
    PDAMessage_Highlight message1(10);
    PDAMessage_Highlight message2;

    std::vector<uint8_t> serializedMessage;
    BOOST_REQUIRE(message1.Serialize(serializedMessage));
    BOOST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(serializedMessage))));

    BOOST_CHECK_EQUAL(message1.Destination(), message2.Destination());
    BOOST_CHECK_EQUAL(message1.Id(), message2.Id());
    BOOST_CHECK_NE(message1.RawId(), message2.RawId()); // Deserialisation captures the message "raw" id...
    BOOST_CHECK_EQUAL(0x08, message2.RawId());
    BOOST_CHECK_NE(message1.MessageLength(), message2.MessageLength());  // Deserialisation captures the message length...
    BOOST_CHECK_EQUAL(10, message2.MessageLength());
    BOOST_CHECK_NE(message1.ChecksumValue(), message2.ChecksumValue());  // Deserialisation captures the message checksum value...
    BOOST_CHECK_EQUAL(0x24, message2.ChecksumValue());

    BOOST_CHECK_EQUAL(10, message2.LineId());
}

BOOST_AUTO_TEST_CASE(TestToString)
{
    PDAMessage_Highlight message;

    std::string expected = "Packet: Destination: AqualinkMaster (0x00), Message Type: PDA_Highlight (0x08) || Payload: 0";

    BOOST_CHECK_EQUAL(message.ToString(), expected);
}

BOOST_AUTO_TEST_SUITE_END()
