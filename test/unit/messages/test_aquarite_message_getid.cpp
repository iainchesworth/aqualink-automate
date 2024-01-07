#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <magic_enum.hpp>

#include "jandy/devices/jandy_device_id.h"
#include "jandy/formatters/jandy_device_formatters.h"
#include "jandy/formatters/jandy_message_formatters.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/aquarite/aquarite_message_getid.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Messages;

BOOST_AUTO_TEST_SUITE(AquariteMessage_GetIdTestSuite)

BOOST_AUTO_TEST_CASE(TestAquariteMessage_GetIdConstruction)
{
    AquariteMessage_GetId message;

    BOOST_CHECK_EQUAL(message.Destination(), JandyDeviceId(0x00));
    BOOST_CHECK_EQUAL(message.Id(), JandyMessageIds::AQUARITE_GetId);
    BOOST_CHECK_EQUAL(message.RawId(), 0x0);
    BOOST_CHECK_EQUAL(message.MessageLength(), 0);
    BOOST_CHECK_EQUAL(message.ChecksumValue(), 0);

    BOOST_CHECK_EQUAL(message.RequestedPanelData(), PanelDataTypes::Unknown);
}

BOOST_AUTO_TEST_CASE(TestAquariteMessage_GetIdConstructionWithParameter)
{
    AquariteMessage_GetId message(PanelDataTypes::PanelRevision);

    BOOST_CHECK_EQUAL(message.Destination(), JandyDeviceId(0x00));
    BOOST_CHECK_EQUAL(message.Id(), JandyMessageIds::AQUARITE_GetId);
    BOOST_CHECK_EQUAL(message.RawId(), 0x0);
    BOOST_CHECK_EQUAL(message.MessageLength(), 0);
    BOOST_CHECK_EQUAL(message.ChecksumValue(), 0);

    BOOST_CHECK_EQUAL(message.RequestedPanelData(), PanelDataTypes::PanelRevision);
}

BOOST_AUTO_TEST_CASE(TestSerializationDeserialization)
{
    AquariteMessage_GetId message1(PanelDataTypes::PanelType);
    AquariteMessage_GetId message2;

    std::vector<uint8_t> serializedMessage;
    BOOST_REQUIRE(message1.Serialize(serializedMessage));
    BOOST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(serializedMessage))));

    BOOST_CHECK_EQUAL(message1.Destination(), message2.Destination());
    BOOST_CHECK_EQUAL(message1.Id(), message2.Id());
    BOOST_CHECK_NE(message1.RawId(), message2.RawId()); // Deserialisation captures the message "raw" id...
    BOOST_CHECK_EQUAL(0x14, message2.RawId());
    BOOST_CHECK_NE(message1.MessageLength(), message2.MessageLength());  // Deserialisation captures the message length...
    BOOST_CHECK_EQUAL(0x0C, message2.MessageLength());
    BOOST_CHECK_NE(message1.ChecksumValue(), message2.ChecksumValue());  // Deserialisation captures the message checksum value...
    BOOST_CHECK_EQUAL(0x10, message2.ChecksumValue());
}

BOOST_AUTO_TEST_CASE(TestToString)
{
    AquariteMessage_GetId message(PanelDataTypes::PanelType);

    std::string expected = "Packet: Destination: AqualinkMaster (0x00), Message Type: AQUARITE_GetId (0x14) || Payload: 0";

    BOOST_CHECK_EQUAL(message.ToString(), expected);
}

BOOST_AUTO_TEST_SUITE_END()
