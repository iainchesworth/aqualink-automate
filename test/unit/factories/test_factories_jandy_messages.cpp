#include <format>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <magic_enum/magic_enum.hpp>

#include "errors/protocol_errors.h"
#include "jandy/factories/jandy_message_factory.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/formatters/jandy_device_formatters.h"
#include "jandy/formatters/jandy_message_formatters.h"
#include "jandy/messages/jandy_message.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_message.h"
#include "jandy/messages/jandy_message_status.h"
#include "jandy/messages/jandy_message_probe.h"
#include "jandy/messages/jandy_message_unknown.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_constants.h"
#include "jandy/messages/aquarite/aquarite_message_setpercent.h"
#include "jandy/utility/jandy_checksum.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate::Factory;
using namespace AqualinkAutomate::Messages;
using namespace AqualinkAutomate::Types;

BOOST_AUTO_TEST_SUITE(JandyMessageFactory_TestSuite)

BOOST_AUTO_TEST_CASE(JandyMessageFactory_FactoryRegistrationCount)
{
    // Verify the factory has registered the expected number of message types
    BOOST_CHECK_GT(JandyMessageFactoryT::RegisteredCount(), 0);
    BOOST_CHECK_EQUAL(JandyMessageFactoryT::RegisteredCount(), 44); // Based on registration file (+AQUARITE_SetPercent 0x15, +IAQ_PageSubMessage 0x27)
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_CreateHotPathMessage_Ack)
{
    auto message = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::Ack);

    BOOST_REQUIRE(message != nullptr);
    BOOST_CHECK_EQUAL(message->Id(), JandyMessageIds::Ack);

    // Verify it's actually an Ack message
    auto ack_message = std::dynamic_pointer_cast<JandyMessage_Ack>(message);
    BOOST_CHECK(ack_message != nullptr);
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_CreateHotPathMessage_Status)
{
    auto message = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::Status);

    BOOST_REQUIRE(message != nullptr);
    BOOST_CHECK_EQUAL(message->Id(), JandyMessageIds::Status);

    auto status_message = std::dynamic_pointer_cast<JandyMessage_Status>(message);
    BOOST_CHECK(status_message != nullptr);
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_CreateHotPathMessage_Probe)
{
    auto message = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::Probe);

    BOOST_REQUIRE(message != nullptr);
    BOOST_CHECK_EQUAL(message->Id(), JandyMessageIds::Probe);

    auto probe_message = std::dynamic_pointer_cast<JandyMessage_Probe>(message);
    BOOST_CHECK(probe_message != nullptr);
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_CreateColdPathMessage_Message)
{
    auto message = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::Message);

    BOOST_REQUIRE(message != nullptr);
    BOOST_CHECK_EQUAL(message->Id(), JandyMessageIds::Message);

    auto msg_message = std::dynamic_pointer_cast<JandyMessage_Message>(message);
    BOOST_CHECK(msg_message != nullptr);
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_CreateColdPathMessage_Unknown)
{
    auto message = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::Unknown);

    BOOST_REQUIRE(message != nullptr);
    BOOST_CHECK_EQUAL(message->Id(), JandyMessageIds::Unknown);

    auto unknown_message = std::dynamic_pointer_cast<JandyMessage_Unknown>(message);
    BOOST_CHECK(unknown_message != nullptr);
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_CreateColdPathMessage_AquariteGetId)
{
    auto message = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::AQUARITE_GetId);

    BOOST_REQUIRE(message != nullptr);
    BOOST_CHECK_EQUAL(message->Id(), JandyMessageIds::AQUARITE_GetId);
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_CreateColdPathMessage_IAQPoll)
{
    auto message = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::IAQ_Poll);

    BOOST_REQUIRE(message != nullptr);
    BOOST_CHECK_EQUAL(message->Id(), JandyMessageIds::IAQ_Poll);
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_CreateColdPathMessage_AquariteSetPercent)
{
    // 0x15 -- the AquaRite second "set output %" command recovered from the simulator.
    auto message = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::AQUARITE_SetPercent);

    BOOST_REQUIRE(message != nullptr);
    BOOST_CHECK_EQUAL(message->Id(), JandyMessageIds::AQUARITE_SetPercent);

    auto setpercent_message = std::dynamic_pointer_cast<AquariteMessage_SetPercent>(message);
    BOOST_CHECK(setpercent_message != nullptr);
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_IAQPageSubMessage_RecognisedAsNamedUnknown)
{
    // 0x27 is registered as a named, payload-retaining Unknown: the factory yields a
    // JandyMessage_Unknown, and deserialising a real 0x27 frame must resolve the id to
    // IAQ_PageSubMessage (not the 0xFF catch-all) with its 5 payload bytes retained.
    auto message = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::IAQ_PageSubMessage);
    BOOST_REQUIRE(message != nullptr);

    auto unknown_message = std::dynamic_pointer_cast<JandyMessage_Unknown>(message);
    BOOST_REQUIRE(unknown_message != nullptr);

    std::vector<uint8_t> frame =
    {
        HEADER_BYTE_DLE, HEADER_BYTE_STX,
        0x33,
        magic_enum::enum_integer(JandyMessageIds::IAQ_PageSubMessage),
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE,
        0x00, // checksum placeholder
        HEADER_BYTE_DLE, HEADER_BYTE_ETX
    };
    frame[9] = AqualinkAutomate::Utility::JandyPacket_CalculateChecksum(frame.begin(), frame.begin() + 9);

    BOOST_REQUIRE(unknown_message->Deserialize(std::as_bytes(std::span<uint8_t>(frame))));
    BOOST_CHECK_EQUAL(unknown_message->Id(), JandyMessageIds::IAQ_PageSubMessage);

    const std::vector<uint8_t> expected_payload = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE };
    BOOST_CHECK_EQUAL_COLLECTIONS(unknown_message->Payload().begin(), unknown_message->Payload().end(), expected_payload.begin(), expected_payload.end());
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_CreateInvalidMessageId)
{
    const uint8_t UNUSED_MESSAGE_ID = 0xFE;
    static_assert(!magic_enum::enum_contains<JandyMessageIds>(UNUSED_MESSAGE_ID), "Reserved value needs to be unused in JandyMessageIds (e.g. 0xFE");

    // Try to create a message with an ID that's not registered
    auto message = JandyMessageFactoryT::CreateMessageFromRaw(static_cast<JandyMessageIds>(UNUSED_MESSAGE_ID));

    BOOST_CHECK(message == nullptr);
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_SerializationRoundTrip_Ack)
{
    // Create and serialize an Ack message
    JandyMessage_Ack original(AckTypes::ACK_IAQTouch, 0x10);
    std::vector<uint8_t> serialized;
    BOOST_REQUIRE(original.Serialize(serialized));

    // Deserialize using factory
    auto result = JandyMessageFactoryT::CreateFromSerialData(std::ranges::subrange(serialized.begin(), serialized.end()));

    BOOST_REQUIRE(result.has_value());
    auto deserialized = result.value();
    BOOST_REQUIRE(deserialized != nullptr);

    // Verify type
    auto ack_message = std::dynamic_pointer_cast<JandyMessage_Ack>(deserialized);
    BOOST_REQUIRE(ack_message != nullptr);

    // Verify contents
    BOOST_CHECK_EQUAL(ack_message->AckType(), AckTypes::ACK_IAQTouch);
    BOOST_CHECK_EQUAL(ack_message->Command(), 0x10);
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_SerializationRoundTrip_Message)
{
    const std::string test_line = "TEST MSG CONTENT";
    JandyMessage_Message original(test_line);
    std::vector<uint8_t> serialized;
    BOOST_REQUIRE(original.Serialize(serialized));

    auto result = JandyMessageFactoryT::CreateFromSerialData(std::ranges::subrange(serialized.begin(), serialized.end()));

    BOOST_REQUIRE(result.has_value());
    auto deserialized = result.value();
    BOOST_REQUIRE(deserialized != nullptr);

    auto msg_message = std::dynamic_pointer_cast<JandyMessage_Message>(deserialized);
    BOOST_REQUIRE(msg_message != nullptr);

    BOOST_CHECK_EQUAL(msg_message->Line(), test_line);
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_CreateFromSerialData_TooShort)
{
    std::vector<uint8_t> short_data = { 0x10 }; // Only 1 byte

    auto result = JandyMessageFactoryT::CreateFromSerialData(std::ranges::subrange(short_data.begin(), short_data.end()));

    BOOST_CHECK(!result.has_value());
    BOOST_CHECK(result.error());
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_CreateFromSerialData_EmptyData)
{
    std::vector<uint8_t> empty_data;

    auto result = JandyMessageFactoryT::CreateFromSerialData(std::ranges::subrange(empty_data.begin(), empty_data.end()));

    BOOST_CHECK(!result.has_value());
    BOOST_CHECK(result.error());
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_CreateFromSerialData_TwoBytes_NoOutOfBoundsRead)
{
    // Regression: previously the size guard accepted any input with >= 2 bytes
    // and then dereferenced index Index_MessageType (3), reading out of bounds for
    // a 2-byte input.  The guard now requires a full MINIMUM_PACKET_LENGTH packet,
    // so this must fail cleanly with an invalid-packet-format error (no OOB read).
    std::vector<uint8_t> two_bytes = { 0x10, 0x02 };

    auto result = JandyMessageFactoryT::CreateFromSerialData(std::ranges::subrange(two_bytes.begin(), two_bytes.end()));

    BOOST_REQUIRE(!result.has_value());
    BOOST_CHECK(result.error() == make_error_code(AqualinkAutomate::ErrorCodes::Protocol_ErrorCodes::InvalidPacketFormat));
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_CreateFromSerialData_ThreeBytes_NoOutOfBoundsRead)
{
    // Regression: a 3-byte input has no byte at index Index_MessageType (3); the
    // guard must reject it before any dereference of the message-type byte.
    std::vector<uint8_t> three_bytes = { 0x10, 0x02, 0x00 };

    auto result = JandyMessageFactoryT::CreateFromSerialData(std::ranges::subrange(three_bytes.begin(), three_bytes.end()));

    BOOST_REQUIRE(!result.has_value());
    BOOST_CHECK(result.error() == make_error_code(AqualinkAutomate::ErrorCodes::Protocol_ErrorCodes::InvalidPacketFormat));
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_CreateFromSerialData_BelowMinimumPacketLength)
{
    // Any input shorter than MINIMUM_PACKET_LENGTH must be rejected by the size guard.
    std::vector<uint8_t> short_packet(JandyMessage::MINIMUM_PACKET_LENGTH - 1, 0x00);

    auto result = JandyMessageFactoryT::CreateFromSerialData(std::ranges::subrange(short_packet.begin(), short_packet.end()));

    BOOST_REQUIRE(!result.has_value());
    BOOST_CHECK(result.error() == make_error_code(AqualinkAutomate::ErrorCodes::Protocol_ErrorCodes::InvalidPacketFormat));
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_CreateFromSerialData_InvalidMessageType)
{
    // Create a packet with an invalid message type
    std::vector<uint8_t> invalid_data = {
        0x10, 0x02,  // Header
        0xFF,        // Invalid message type
        0x00,        // Destination
        0x10, 0x03   // Footer
    };

    auto result = JandyMessageFactoryT::CreateFromSerialData(std::ranges::subrange(invalid_data.begin(), invalid_data.end()));

    BOOST_CHECK(!result.has_value());
    BOOST_CHECK(result.error());
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_CreateFromSerialData_CorruptedChecksum)
{
    // Create a valid message then corrupt it
    JandyMessage_Ack original(AckTypes::ACK_IAQTouch, 0x10);
    std::vector<uint8_t> serialized;
    BOOST_REQUIRE(original.Serialize(serialized));

    // Corrupt the checksum (last byte before footer)
    if (serialized.size() > 2) 
    {
        serialized[serialized.size() - 2] ^= 0xFF;
    }

    auto result = JandyMessageFactoryT::CreateFromSerialData(std::ranges::subrange(serialized.begin(), serialized.end()));

    // Should fail due to invalid checksum
    BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_CreateMultipleMessages_SameType)
{
    // Verify factory can create multiple instances of the same type
    auto msg1 = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::Ack);
    auto msg2 = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::Ack);
    auto msg3 = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::Ack);

    BOOST_REQUIRE(msg1 != nullptr);
    BOOST_REQUIRE(msg2 != nullptr);
    BOOST_REQUIRE(msg3 != nullptr);

    // Verify they are separate instances
    BOOST_CHECK(msg1.get() != msg2.get());
    BOOST_CHECK(msg2.get() != msg3.get());
    BOOST_CHECK(msg1.get() != msg3.get());
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_CreateMultipleMessages_DifferentTypes)
{
    auto ack = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::Ack);
    auto status = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::Status);
    auto probe = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::Probe);
    auto message = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::Message);

    BOOST_REQUIRE(ack != nullptr);
    BOOST_REQUIRE(status != nullptr);
    BOOST_REQUIRE(probe != nullptr);
    BOOST_REQUIRE(message != nullptr);

    BOOST_CHECK_EQUAL(ack->Id(), JandyMessageIds::Ack);
    BOOST_CHECK_EQUAL(status->Id(), JandyMessageIds::Status);
    BOOST_CHECK_EQUAL(probe->Id(), JandyMessageIds::Probe);
    BOOST_CHECK_EQUAL(message->Id(), JandyMessageIds::Message);
}

BOOST_AUTO_TEST_CASE(JandyMessageFactory_CreateMessage_AllRegisteredTypes)
{
    // Test that all registered message types can be created
    std::vector<JandyMessageIds> registered_ids = {
        JandyMessageIds::Ack,
        JandyMessageIds::Status,
        JandyMessageIds::Probe,
        JandyMessageIds::Message,
        JandyMessageIds::MessageLong,
        JandyMessageIds::Unknown,
        JandyMessageIds::AQUARITE_GetId,
        JandyMessageIds::IAQ_Poll
        // Add more as needed
    };

    for (const auto& id : registered_ids) {
        auto message = JandyMessageFactoryT::CreateMessageFromRaw(id);
        BOOST_CHECK_MESSAGE(message != nullptr,
            std::format("Failed to create message for type: {}", static_cast<int>(id)));
    }
}

BOOST_AUTO_TEST_SUITE_END()
