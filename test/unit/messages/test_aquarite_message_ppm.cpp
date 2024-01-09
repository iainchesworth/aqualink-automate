#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <magic_enum.hpp>

#include "jandy/devices/jandy_device_id.h"
#include "jandy/formatters/jandy_device_formatters.h"
#include "jandy/formatters/jandy_message_formatters.h"
#include "jandy/messages/jandy_message_constants.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/aquarite/aquarite_message_ppm.h"
#include "jandy/utility/jandy_checksum.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Messages;

BOOST_AUTO_TEST_SUITE(AquariteMessage_PPMTestSuite)

BOOST_AUTO_TEST_CASE(TestAquariteMessage_PPMConstruction)
{
    AquariteMessage_PPM message;

    BOOST_CHECK_EQUAL(message.Destination(), JandyDeviceId(0x00));
    BOOST_CHECK_EQUAL(message.Id(), JandyMessageIds::AQUARITE_PPM);
    BOOST_CHECK_EQUAL(message.RawId(), 0x0);
    BOOST_CHECK_EQUAL(message.MessageLength(), 0);
    BOOST_CHECK_EQUAL(message.ChecksumValue(), 0);

    BOOST_CHECK_EQUAL(message.SaltConcentrationPPM(), 0);
    BOOST_CHECK_EQUAL(message.Status(), AquariteStatuses::Unknown);
}

BOOST_AUTO_TEST_CASE(TestSerializationDeserialization)
{
    AquariteMessage_PPM message1;
    AquariteMessage_PPM message2;

    std::vector<uint8_t> serializedMessage;
    BOOST_REQUIRE(message1.Serialize(serializedMessage));
    BOOST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(serializedMessage))));

    BOOST_CHECK_EQUAL(message1.Destination(), message2.Destination());
    BOOST_CHECK_EQUAL(message1.Id(), message2.Id());
    BOOST_CHECK_NE(message1.RawId(), message2.RawId()); // Deserialisation captures the message "raw" id...
    BOOST_CHECK_EQUAL(0x16, message2.RawId());
    BOOST_CHECK_NE(message1.MessageLength(), message2.MessageLength());  // Deserialisation captures the message length...
    BOOST_CHECK_EQUAL(0x0D, message2.MessageLength());
    BOOST_CHECK_NE(message1.ChecksumValue(), message2.ChecksumValue());  // Deserialisation captures the message checksum value...
    BOOST_CHECK_EQUAL(0x10, message2.ChecksumValue());

    BOOST_CHECK_EQUAL(0x00, message1.SaltConcentrationPPM());
    BOOST_CHECK_EQUAL(0x00, message2.SaltConcentrationPPM());

    std::vector<uint8_t> message_bytes =
    {
        HEADER_BYTE_DLE,
        HEADER_BYTE_STX,
        0x00,
        magic_enum::enum_integer(JandyMessageIds::AQUARITE_PPM),
        0x00,
        magic_enum::enum_integer(AquariteStatuses::Unknown),
        0x00,
        HEADER_BYTE_DLE,
        HEADER_BYTE_ETX
    };

    message_bytes[4] = 0x00;
    message_bytes[5] = magic_enum::enum_integer(AquariteStatuses::On);
    {
        auto message_span_to_checksum = std::as_bytes(std::span<uint8_t>(message_bytes.begin(), 6));
        message_bytes[6] = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(message_span_to_checksum);
        BOOST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(message_bytes))));
        BOOST_CHECK_EQUAL(0, message2.SaltConcentrationPPM());
        BOOST_CHECK_EQUAL(AquariteStatuses::On, message2.Status());
    }

    message_bytes[4] = 0x1E;
    message_bytes[5] = magic_enum::enum_integer(AquariteStatuses::Warning_LowSalt);
    {
        auto message_span_to_checksum = std::as_bytes(std::span<uint8_t>(message_bytes.begin(), 6));
        message_bytes[6] = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(message_span_to_checksum);
        BOOST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(message_bytes))));
        BOOST_CHECK_EQUAL(3000, message2.SaltConcentrationPPM());
        BOOST_CHECK_EQUAL(AquariteStatuses::Warning_LowSalt, message2.Status());
    }

    message_bytes[4] = 0x28;
    message_bytes[5] = magic_enum::enum_integer(AquariteStatuses::TurningOff);
    {
        auto message_span_to_checksum = std::as_bytes(std::span<uint8_t>(message_bytes.begin(), 6));
        message_bytes[6] = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(message_span_to_checksum);
        BOOST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(message_bytes))));
        BOOST_CHECK_EQUAL(4000, message2.SaltConcentrationPPM());
        BOOST_CHECK_EQUAL(AquariteStatuses::TurningOff, message2.Status());
    }

    message_bytes[4] = 0xFF;
    message_bytes[5] = magic_enum::enum_integer(AquariteStatuses::Off);
    {
        auto message_span_to_checksum = std::as_bytes(std::span<uint8_t>(message_bytes.begin(), 6));
        message_bytes[6] = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(message_span_to_checksum);
        BOOST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(message_bytes))));
        BOOST_CHECK_EQUAL(25500, message2.SaltConcentrationPPM());
        BOOST_CHECK_EQUAL(AquariteStatuses::Off, message2.Status());
    }
}

BOOST_AUTO_TEST_CASE(TestDeserialization_MessageMissingPayload)
{
    std::vector<std::vector<uint8_t>> message_bytes =
    {
        {
            HEADER_BYTE_DLE,
            HEADER_BYTE_STX,
            0x00,
            // 
            // The following would be the "expected" bytes...
            // 
            // magic_enum::enum_integer(JandyMessageIds::AQUARITE_PPM),
            // 0x00,
            // magic_enum::enum_integer(AquariteStatuses::Unknown),
            // <checksum>,
            // HEADER_BYTE_DLE,
            // HEADER_BYTE_ETX
        },
        {
            HEADER_BYTE_DLE,
            HEADER_BYTE_STX,
            0x00,
            magic_enum::enum_integer(JandyMessageIds::AQUARITE_PPM),
            // 
            // The following would be the "expected" bytes...
            // 
            // 0x00,
            // magic_enum::enum_integer(AquariteStatuses::Unknown),
            // <checksum>
            // HEADER_BYTE_DLE
            // HEADER_BYTE_ETX
        },
        {
            HEADER_BYTE_DLE,
            HEADER_BYTE_STX,
            0x00,
            magic_enum::enum_integer(JandyMessageIds::AQUARITE_PPM),
            0x00,
            // 
            // The following would be the "expected" bytes...
            // 
            // magic_enum::enum_integer(AquariteStatuses::Unknown),
            // <checksum>
            // HEADER_BYTE_DLE
            // HEADER_BYTE_ETX
        },
        {
            HEADER_BYTE_DLE,
            HEADER_BYTE_STX,
            0x00,
            magic_enum::enum_integer(JandyMessageIds::AQUARITE_PPM),
            0x00,
            magic_enum::enum_integer(AquariteStatuses::Unknown),
            // 
            // The following would be the "expected" bytes...
            // 
            // <checksum>
            // HEADER_BYTE_DLE
            // HEADER_BYTE_ETX
        }
    };

    AquariteMessage_PPM message;

    BOOST_CHECK_EQUAL(false, message.DeserializeContents(message_bytes[0]));
    BOOST_CHECK_EQUAL(false, message.DeserializeContents(message_bytes[1]));
    BOOST_CHECK_EQUAL(false, message.DeserializeContents(message_bytes[2]));
    BOOST_CHECK_EQUAL(true, message.DeserializeContents(message_bytes[3]));
}

BOOST_AUTO_TEST_CASE(TestToString)
{
    AquariteMessage_PPM message;

    std::string expected = "Packet: Destination: AqualinkMaster (0x00), Message Type: AQUARITE_PPM (0x16) || Payload: PPM: 0, Status: Unknown";

    BOOST_CHECK_EQUAL(message.ToString(), expected);
}

BOOST_AUTO_TEST_SUITE_END()
