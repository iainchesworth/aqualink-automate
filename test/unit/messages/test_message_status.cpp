#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/formatters/jandy_device_formatters.h"
#include "jandy/formatters/jandy_message_formatters.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_status.h"
#include "kernel/auxillary_devices/auxillary_status.h"
#include "kernel/auxillary_devices/chlorinator_status.h"
#include "kernel/auxillary_devices/heater_status.h"
#include "kernel/auxillary_devices/pump_status.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate::Kernel;
using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Messages;

BOOST_AUTO_TEST_SUITE(JandyMessage_StatusTestSuite)

BOOST_AUTO_TEST_CASE(TestJandyMessage_StatusConstruction)
{
    JandyMessage_Status message;
    BOOST_CHECK_EQUAL(message.Destination(), JandyDeviceId(0x00));
    BOOST_CHECK_EQUAL(message.Id(), JandyMessageIds::Status);
    BOOST_CHECK_EQUAL(message.RawId(), 0x0);
    BOOST_CHECK_EQUAL(message.MessageLength(), 0);
    BOOST_CHECK_EQUAL(message.ChecksumValue(), 0);

    // Check default payload values.
    BOOST_CHECK_EQUAL(message.Mode(), ComboModes::Unknown);
    BOOST_CHECK_EQUAL(message.FilterPump(), PumpStatuses::Unknown);
    BOOST_CHECK_EQUAL(message.Aux1(), AuxillaryStatuses::Unknown);
    BOOST_CHECK_EQUAL(message.Aux2(), AuxillaryStatuses::Unknown);
    BOOST_CHECK_EQUAL(message.Aux3(), AuxillaryStatuses::Unknown);
    BOOST_CHECK_EQUAL(message.Aux4(), AuxillaryStatuses::Unknown);
    BOOST_CHECK_EQUAL(message.Aux5(), AuxillaryStatuses::Unknown);
    BOOST_CHECK_EQUAL(message.Aux6(), AuxillaryStatuses::Unknown);
    BOOST_CHECK_EQUAL(message.Aux7(), AuxillaryStatuses::Unknown);
    BOOST_CHECK_EQUAL(message.PoolHeater(), HeaterStatuses::Unknown);
    BOOST_CHECK_EQUAL(message.SpaHeater(), HeaterStatuses::Unknown);
    BOOST_CHECK_EQUAL(message.SolarHeater(), HeaterStatuses::Unknown);
}

BOOST_AUTO_TEST_CASE(TestSerializationDeserialization)
{
    JandyMessage_Status message1;
    JandyMessage_Status message2;

    std::vector<uint8_t> serializedMessage;
    BOOST_REQUIRE(message1.Serialize(serializedMessage));
    BOOST_REQUIRE(message2.Deserialize(std::as_bytes(std::span<uint8_t>(serializedMessage))));

    BOOST_CHECK_EQUAL(message1.Destination(), message2.Destination());
    BOOST_CHECK_EQUAL(message1.Id(), message2.Id());
    BOOST_CHECK_NE(message1.RawId(), message2.RawId()); // Deserialisation captures the message "raw" id...
    BOOST_CHECK_EQUAL(0x02, message2.RawId());
    BOOST_CHECK_NE(message1.MessageLength(), message2.MessageLength());  // Deserialisation captures the message length...
    BOOST_CHECK_EQUAL(12, message2.MessageLength());
    BOOST_CHECK_NE(message1.ChecksumValue(), message2.ChecksumValue());  // Deserialisation captures the message checksum value...
    BOOST_CHECK_EQUAL(0x14, message2.ChecksumValue());  // Checksum unchanged (added zero byte)

    //
    // NOTE: A default constructed status message marks it's contents as "Unknown" or the specific value
    // for that specific field.  Doing a serialization <--> deserialization means that those values will
    // changed to the equivalent of '0x00' due to the serialization method merely pushing NUL bytes into
    // the payload.  Thus, we cannot show that the message is equivalent...see below for what to check.
    //

    // Check payload values.
    BOOST_CHECK_NE(message1.Mode(), message2.Mode());
    BOOST_CHECK_NE(message1.FilterPump(), message2.FilterPump());
    BOOST_CHECK_NE(message1.Aux1(), message2.Aux1());
    BOOST_CHECK_NE(message1.Aux2(), message2.Aux2());
    BOOST_CHECK_NE(message1.Aux3(), message2.Aux3());
    BOOST_CHECK_NE(message1.Aux4(), message2.Aux4());
    BOOST_CHECK_NE(message1.Aux5(), message2.Aux5());
    BOOST_CHECK_NE(message1.Aux6(), message2.Aux6());
    BOOST_CHECK_NE(message1.Aux7(), message2.Aux7());
    BOOST_CHECK_NE(message1.PoolHeater(), message2.PoolHeater());
    BOOST_CHECK_EQUAL(message1.SpaHeater(), message2.SpaHeater());       // Both Unknown: 12-byte packet has no extended payload
    BOOST_CHECK_EQUAL(message1.SolarHeater(), message2.SolarHeater());   // Both Unknown: 12-byte packet has no extended payload

    // Check payload values.
    BOOST_CHECK_EQUAL(AqualinkAutomate::Messages::ComboModes::Pool, message2.Mode());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::PumpStatuses::Off, message2.FilterPump());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::AuxillaryStatuses::On, message2.Aux1());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::AuxillaryStatuses::On, message2.Aux2());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::AuxillaryStatuses::On, message2.Aux3());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::AuxillaryStatuses::On, message2.Aux4());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::AuxillaryStatuses::On, message2.Aux5());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::AuxillaryStatuses::On, message2.Aux6());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::AuxillaryStatuses::On, message2.Aux7());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::HeaterStatuses::Off, message2.PoolHeater());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::HeaterStatuses::Unknown, message2.SpaHeater());     // 12-byte packet: no extended payload
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::HeaterStatuses::Unknown, message2.SolarHeater()); // 12-byte packet: no extended payload
}

BOOST_AUTO_TEST_CASE(TestToString)
{
    JandyMessage_Status message;

    const std::string expected = "Packet: Destination: AqualinkMaster (0x00), Message Type: Status (0x02) || Payload: 0";

    BOOST_CHECK_EQUAL(message.ToString(), expected);
}

BOOST_AUTO_TEST_CASE(TestHeaterStatuses_DecodedFromPayloadNotChecksum)
{
    // Regression test: SolarHeater and SpaHeater must be decoded from
    // payload byte [9], not from the checksum byte.
    // Build a 13-byte packet: DLE STX Dest MsgType [6 payload bytes] Checksum DLE ETX
    //
    // Payload byte layout (indices 4-9):
    //   [4]=0x00 (unused), [5]=aux bits, [6]=pump/mode/aux1, [7]=aux6/aux4,
    //   [8]=PoolHeater bits, [9]=SolarHeater+SpaHeater bits

    // Set byte[9] to encode SolarHeater=Heating(0x01) and SpaHeater=Enabled(0x04)
    // SolarHeater: (byte & 0x70) >> 4 = 0x01 => byte has 0x10 in bits 6-4
    // SpaHeater:   (byte & 0x07) >> 0 = 0x04 => byte has 0x04 in bits 2-0
    // So byte[9] = 0x14

    std::vector<uint8_t> packet = {
        0x10, 0x02,       // DLE STX
        0x00,             // Destination
        0x02,             // Message Type (Status)
        0x00,             // payload[0] - unused
        0x00,             // payload[1] - aux bits
        0x00,             // payload[2] - pump/mode/aux1
        0x00,             // payload[3] - aux6/aux4
        0x00,             // payload[4] - PoolHeater
        0x14,             // payload[5] - SolarHeater=Heating, SpaHeater=Enabled
        0x00,             // checksum placeholder
        0x10, 0x03        // DLE ETX
    };

    // Calculate correct checksum (sum of bytes 0 through N-3)
    uint8_t checksum = 0;
    for (size_t i = 0; i < packet.size() - 3; ++i)
    {
        checksum += packet[i];
    }
    packet[packet.size() - 3] = checksum;

    JandyMessage_Status message;
    BOOST_REQUIRE(message.Deserialize(std::as_bytes(std::span<uint8_t>(packet))));

    // These must come from payload byte[9]=0x14, NOT the checksum
    BOOST_CHECK_EQUAL(message.SolarHeater(), HeaterStatuses::Heating);
    BOOST_CHECK_EQUAL(message.SpaHeater(), HeaterStatuses::Enabled);
    BOOST_CHECK_EQUAL(message.PoolHeater(), HeaterStatuses::Off);
}

BOOST_AUTO_TEST_SUITE_END()
