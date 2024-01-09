#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <magic_enum.hpp>

#include "jandy/devices/jandy_device_id.h"
#include "jandy/formatters/jandy_device_formatters.h"
#include "jandy/formatters/jandy_message_formatters.h"
#include "jandy/messages/jandy_message_constants.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/aquarite/aquarite_message_percent.h"
#include "jandy/utility/jandy_checksum.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Messages;

BOOST_AUTO_TEST_SUITE(AquariteMessage_PercentTestSuite)

BOOST_AUTO_TEST_CASE(TestAquariteMessage_PercentConstruction)
{
    AquariteMessage_Percent message;

    BOOST_CHECK_EQUAL(message.Destination(), JandyDeviceId(0x00));
    BOOST_CHECK_EQUAL(message.Id(), JandyMessageIds::AQUARITE_Percent);
    BOOST_CHECK_EQUAL(message.RawId(), 0x0);
    BOOST_CHECK_EQUAL(message.MessageLength(), 0);
    BOOST_CHECK_EQUAL(message.ChecksumValue(), 0);

    BOOST_CHECK_EQUAL(message.GeneratingPercentage(), 0);
}

BOOST_AUTO_TEST_CASE(TestSerializationDeserialization)
{
    AquariteMessage_Percent message1;
    AquariteMessage_Percent message2;

    std::vector<uint8_t> serializedMessage;
    BOOST_REQUIRE(message1.Serialize(serializedMessage));
    BOOST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(serializedMessage))));

    BOOST_CHECK_EQUAL(message1.Destination(), message2.Destination());
    BOOST_CHECK_EQUAL(message1.Id(), message2.Id());
    BOOST_CHECK_NE(message1.RawId(), message2.RawId()); // Deserialisation captures the message "raw" id...
    BOOST_CHECK_EQUAL(0x11, message2.RawId());
    BOOST_CHECK_NE(message1.MessageLength(), message2.MessageLength());  // Deserialisation captures the message length...
    BOOST_CHECK_EQUAL(0x0C, message2.MessageLength());
    BOOST_CHECK_NE(message1.ChecksumValue(), message2.ChecksumValue());  // Deserialisation captures the message checksum value...
    BOOST_CHECK_EQUAL(0x10, message2.ChecksumValue());

    BOOST_CHECK_EQUAL(0x00, message1.GeneratingPercentage());
    BOOST_CHECK_EQUAL(0x00, message2.GeneratingPercentage());
    
	std::vector<uint8_t> message_bytes =
	{
		HEADER_BYTE_DLE,
		HEADER_BYTE_STX,
		0x00,
		magic_enum::enum_integer(JandyMessageIds::AQUARITE_Percent),
		0x00,
		0x00,
		HEADER_BYTE_DLE,
		HEADER_BYTE_ETX
	};

    message_bytes[4] = 0x00;
    {
        auto message_span_to_checksum = std::as_bytes(std::span<uint8_t>(message_bytes.begin(), 5));
        message_bytes[5] = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(message_span_to_checksum);
        BOOST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(message_bytes))));
        BOOST_CHECK_EQUAL(0x00, message2.GeneratingPercentage());
    }

    message_bytes[4] = 0x32;
    {
        auto message_span_to_checksum = std::as_bytes(std::span<uint8_t>(message_bytes.begin(), 5));
        message_bytes[5] = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(message_span_to_checksum);
        BOOST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(message_bytes))));
        BOOST_CHECK_EQUAL(0x32, message2.GeneratingPercentage());
    }

    message_bytes[4] = 0x64;
    {
        auto message_span_to_checksum = std::as_bytes(std::span<uint8_t>(message_bytes.begin(), 5));
        message_bytes[5] = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(message_span_to_checksum);
        BOOST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(message_bytes))));
        BOOST_CHECK_EQUAL(0x64, message2.GeneratingPercentage());
    }

    message_bytes[4] = 0xFF;
    {
        auto message_span_to_checksum = std::as_bytes(std::span<uint8_t>(message_bytes.begin(), 5));
        message_bytes[5] = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(message_span_to_checksum);
        BOOST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(message_bytes))));
        BOOST_CHECK_EQUAL(0xFF, message2.GeneratingPercentage());
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
            // magic_enum::enum_integer(JandyMessageIds::AQUARITE_Percent),
            // 0x32,
            // <checksum>,
            // HEADER_BYTE_DLE,
            // HEADER_BYTE_ETX
        },
        {
            HEADER_BYTE_DLE,
            HEADER_BYTE_STX,
            0x00,
            magic_enum::enum_integer(JandyMessageIds::AQUARITE_Percent),
            // 
            // The following would be the "expected" bytes...
            // 
            // 0x32,
            // <checksum>
            // HEADER_BYTE_DLE
            // HEADER_BYTE_ETX
        },
        {
            HEADER_BYTE_DLE,
            HEADER_BYTE_STX,
            0x00,
            magic_enum::enum_integer(JandyMessageIds::AQUARITE_Percent),
            0x32,
            // 
            // The following would be the "expected" bytes...
            // 
            // <checksum>
            // HEADER_BYTE_DLE
            // HEADER_BYTE_ETX
        }
    };

    AquariteMessage_Percent message;

    BOOST_CHECK_EQUAL(false, message.DeserializeContents(message_bytes[0]));
    BOOST_CHECK_EQUAL(false, message.DeserializeContents(message_bytes[1]));
    BOOST_CHECK_EQUAL(true, message.DeserializeContents(message_bytes[2]));
}

BOOST_AUTO_TEST_CASE(TestToString)
{
    AquariteMessage_Percent message;

    std::string expected = "Packet: Destination: AqualinkMaster (0x00), Message Type: AQUARITE_Percent (0x11) || Payload: Percent: 0";

    BOOST_CHECK_EQUAL(message.ToString(), expected);
}

BOOST_AUTO_TEST_SUITE_END()
