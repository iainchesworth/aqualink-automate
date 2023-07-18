#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "kernel/auxillary.h"
#include "kernel/heater.h"
#include "kernel/pump.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/formatters/jandy_device_formatters.h"
#include "jandy/formatters/jandy_message_formatters.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_status.h"

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
    BOOST_CHECK_EQUAL(message.FilterPump(), PumpStatus::Unknown);
    BOOST_CHECK_EQUAL(message.Aux1(), AuxillaryStatuses::Unknown);
    BOOST_CHECK_EQUAL(message.Aux2(), AuxillaryStatuses::Unknown);
    BOOST_CHECK_EQUAL(message.Aux3(), AuxillaryStatuses::Unknown);
    BOOST_CHECK_EQUAL(message.Aux4(), AuxillaryStatuses::Unknown);
    BOOST_CHECK_EQUAL(message.Aux5(), AuxillaryStatuses::Unknown);
    BOOST_CHECK_EQUAL(message.Aux6(), AuxillaryStatuses::Unknown);
    BOOST_CHECK_EQUAL(message.Aux7(), AuxillaryStatuses::Unknown);
    BOOST_CHECK_EQUAL(message.PoolHeater(), HeaterStatus::Unknown);
    BOOST_CHECK_EQUAL(message.SpaHeater(), HeaterStatus::Unknown);
    BOOST_CHECK_EQUAL(message.SolarHeater(), HeaterStatus::Unknown);
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
    BOOST_CHECK_EQUAL(0x14, message2.ChecksumValue());

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
    BOOST_CHECK_NE(message1.SpaHeater(), message2.SpaHeater());
    BOOST_CHECK_NE(message1.SolarHeater(), message2.SolarHeater());

    // Check payload values.
    BOOST_CHECK_EQUAL(AqualinkAutomate::Messages::ComboModes::Pool, message2.Mode());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::PumpStatus::Off, message2.FilterPump());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::AuxillaryStatuses::On, message2.Aux1());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::AuxillaryStatuses::On, message2.Aux2());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::AuxillaryStatuses::On, message2.Aux3());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::AuxillaryStatuses::On, message2.Aux4());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::AuxillaryStatuses::On, message2.Aux5());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::AuxillaryStatuses::On, message2.Aux6());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::AuxillaryStatuses::On, message2.Aux7());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::HeaterStatus::Off, message2.PoolHeater());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::HeaterStatus::Enabled, message2.SpaHeater());
    BOOST_CHECK_EQUAL(AqualinkAutomate::Kernel::HeaterStatus::Heating, message2.SolarHeater());
}

BOOST_AUTO_TEST_CASE(TestToString)
{
    JandyMessage_Status message;

    const std::string expected = "Packet: Destination: AqualinkMaster (0x00), Message Type: Status (0x02) || Payload: 0";

    BOOST_CHECK_EQUAL(message.ToString(), expected);
}

BOOST_AUTO_TEST_SUITE_END()
