#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <magic_enum/magic_enum.hpp>

#include "jandy/devices/jandy_device_id.h"
#include "jandy/formatters/jandy_device_formatters.h"
#include "jandy/formatters/jandy_message_formatters.h"
#include "jandy/messages/jandy_message_constants.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/aquarite/aquarite_message_setpercent.h"
#include "jandy/utility/jandy_checksum.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Messages;

// AQUARITE_SetPercent (0x15) -- the second "set output %" command recovered from the
// Jandy RS simulator (handled identically to AQUARITE_Percent 0x11 by the firmware).
// Modelled as a percent message; this suite mirrors the AQUARITE_Percent tests.

BOOST_AUTO_TEST_SUITE(AquariteMessage_SetPercentTestSuite)

BOOST_AUTO_TEST_CASE(TestAquariteMessage_SetPercentConstruction)
{
    AquariteMessage_SetPercent message;

    BOOST_CHECK_EQUAL(message.Destination(), JandyDeviceId(0x00));
    BOOST_CHECK_EQUAL(message.Id(), JandyMessageIds::AQUARITE_SetPercent);
    BOOST_CHECK_EQUAL(message.RawId(), 0x0);
    BOOST_CHECK_EQUAL(message.MessageLength(), 0);
    BOOST_CHECK_EQUAL(message.ChecksumValue(), 0);

    BOOST_CHECK_EQUAL(message.GeneratingPercentage(), 0);
}

BOOST_AUTO_TEST_CASE(TestSerializationDeserialization)
{
    AquariteMessage_SetPercent message1;
    AquariteMessage_SetPercent message2;

    std::vector<uint8_t> serializedMessage;
    BOOST_REQUIRE(message1.Serialize(serializedMessage));
    BOOST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(serializedMessage))));

    BOOST_CHECK_EQUAL(message1.Destination(), message2.Destination());
    BOOST_CHECK_EQUAL(message1.Id(), message2.Id());
    BOOST_CHECK_NE(message1.RawId(), message2.RawId()); // Deserialisation captures the message "raw" id...
    BOOST_CHECK_EQUAL(0x15, message2.RawId());

    std::vector<uint8_t> message_bytes =
    {
        HEADER_BYTE_DLE,
        HEADER_BYTE_STX,
        0x00,
        magic_enum::enum_integer(JandyMessageIds::AQUARITE_SetPercent),
        0x00,
        0x00,
        HEADER_BYTE_DLE,
        HEADER_BYTE_ETX
    };

    for (const uint8_t percent : { static_cast<uint8_t>(0x00), static_cast<uint8_t>(0x32), static_cast<uint8_t>(0x64), static_cast<uint8_t>(0xFF) })
    {
        message_bytes[4] = percent;
        message_bytes[5] = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(message_bytes.begin(), message_bytes.begin() + 5);
        BOOST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(message_bytes))));
        BOOST_CHECK_EQUAL(percent, message2.GeneratingPercentage());
    }

    // Boost (101) and Service (0xFF) helpers.
    message_bytes[4] = AquariteMessage_SetPercent::Value_Boost;
    message_bytes[5] = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(message_bytes.begin(), message_bytes.begin() + 5);
    BOOST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(message_bytes))));
    BOOST_CHECK(message2.IsBoostMode());
    BOOST_CHECK(!message2.IsServiceMode());

    message_bytes[4] = AquariteMessage_SetPercent::Value_ServiceMode;
    message_bytes[5] = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(message_bytes.begin(), message_bytes.begin() + 5);
    BOOST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(message_bytes))));
    BOOST_CHECK(message2.IsServiceMode());
    BOOST_CHECK(!message2.IsBoostMode());
}

BOOST_AUTO_TEST_CASE(TestDeserialization_MessageMissingPayload)
{
    std::vector<std::vector<uint8_t>> message_bytes =
    {
        { HEADER_BYTE_DLE, HEADER_BYTE_STX, 0x00 },
        { HEADER_BYTE_DLE, HEADER_BYTE_STX, 0x00, magic_enum::enum_integer(JandyMessageIds::AQUARITE_SetPercent) },
        { HEADER_BYTE_DLE, HEADER_BYTE_STX, 0x00, magic_enum::enum_integer(JandyMessageIds::AQUARITE_SetPercent), 0x32 }
    };

    AquariteMessage_SetPercent message;

    BOOST_CHECK_EQUAL(false, message.DeserializeContents(message_bytes[0]));
    BOOST_CHECK_EQUAL(false, message.DeserializeContents(message_bytes[1]));
    BOOST_CHECK_EQUAL(true, message.DeserializeContents(message_bytes[2]));
}

BOOST_AUTO_TEST_CASE(TestToString)
{
    AquariteMessage_SetPercent message;

    std::string expected = "Packet: Destination: AqualinkMaster (0x00), Message Type: AQUARITE_SetPercent (0x15) || Payload: Percent: 0";

    BOOST_CHECK_EQUAL(message.ToString(), expected);
}

BOOST_AUTO_TEST_SUITE_END()
