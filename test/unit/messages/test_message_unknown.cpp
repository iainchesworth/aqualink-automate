#include <span>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/formatters/jandy_device_formatters.h"
#include "jandy/formatters/jandy_message_formatters.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_unknown.h"

using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Messages;

BOOST_AUTO_TEST_SUITE(JandyMessage_UnknownTestSuite)

BOOST_AUTO_TEST_CASE(TestJandyMessage_UnknownConstruction)
{
    JandyMessage_Unknown message;
    BOOST_CHECK_EQUAL(message.Destination(), JandyDeviceId(0x00));
    BOOST_CHECK_EQUAL(message.Id(), JandyMessageIds::Unknown);
    BOOST_CHECK_EQUAL(message.RawId(), 0x0);
    BOOST_CHECK_EQUAL(message.MessageLength(), 0);
    BOOST_CHECK_EQUAL(message.ChecksumValue(), 0);
}

BOOST_AUTO_TEST_CASE(TestSerializationDeserialization)
{
    JandyMessage_Unknown message1;
    JandyMessage_Unknown message2;

    std::vector<uint8_t> serializedMessage;
    BOOST_REQUIRE(message1.Serialize(serializedMessage));
    BOOST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(serializedMessage))));

    BOOST_CHECK_EQUAL(message1.Destination(), message2.Destination());
    BOOST_CHECK_EQUAL(message1.Id(), message2.Id());
    BOOST_CHECK_NE(message1.RawId(), message2.RawId()); // Deserialisation captures the message "raw" id...
    BOOST_CHECK_EQUAL(0xFF, message2.RawId());
    BOOST_CHECK_NE(message1.MessageLength(), message2.MessageLength());  // Deserialisation captures the message length...
    BOOST_CHECK_EQUAL(7, message2.MessageLength());
    BOOST_CHECK_NE(message1.ChecksumValue(), message2.ChecksumValue());  // Deserialisation captures the message checksum value...
    BOOST_CHECK_EQUAL(0x11, message2.ChecksumValue());
}

BOOST_AUTO_TEST_CASE(TestToString)
{
    JandyMessage_Unknown message;

    const std::string expected = "Packet: Destination: AqualinkMaster (0x00), Message Type: Unknown (0xff) || Payload (0 bytes)";

    BOOST_CHECK_EQUAL(expected, message.ToString());
}

BOOST_AUTO_TEST_CASE(TestDeserialize_RetainsPayload)
{
    // A frame addressed to the master (0x00) carrying a 2-byte payload -- exactly the kind of
    // undecoded "command" the to-master decoder must be able to inspect byte-for-byte. The
    // Unknown message must now retain the payload verbatim (previously it was discarded).
    //   DLE STX 00 29  11 1c  <chk=0x68> DLE ETX   (chk = 0x10+0x02+0x00+0x29+0x11+0x1c = 0x68)
    std::vector<uint8_t> frame = { 0x10, 0x02, 0x00, 0x29, 0x11, 0x1c, 0x68, 0x10, 0x03 };

    JandyMessage_Unknown message;
    BOOST_REQUIRE(message.Deserialize(std::as_bytes(std::span<uint8_t>(frame))));

    BOOST_CHECK_EQUAL(message.RawId(), 0x29);
    BOOST_REQUIRE_EQUAL(message.Payload().size(), 2u);
    BOOST_CHECK_EQUAL(message.Payload()[0], 0x11);
    BOOST_CHECK_EQUAL(message.Payload()[1], 0x1c);
}

BOOST_AUTO_TEST_SUITE_END()
