#include <cstdint>
#include <memory>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/serial_adapter_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/messages/serial_adapter/serial_adapter_message_dev_status.h"
#include "jandy/auxillaries/jandy_auxillary_id.h"

#include "support/unit_test_hublocatorinjector.h"

using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Messages;

namespace
{
	struct SerialAdapterDeviceFixture : public AqualinkAutomate::Test::HubLocatorInjector
	{
		SerialAdapterDeviceFixture()
			: device_type(std::make_shared<JandyDeviceType>(JandyDeviceId(0x48)))
		{
		}

		std::shared_ptr<JandyDeviceType> device_type;
	};
}

BOOST_FIXTURE_TEST_SUITE(SerialAdapterDevice_TestSuite, SerialAdapterDeviceFixture)

// =============================================================================
// Construction
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConstruction_Emulated)
{
	BOOST_CHECK_NO_THROW(SerialAdapterDevice device(device_type, *this, true));
}

BOOST_AUTO_TEST_CASE(TestConstruction_NonEmulated)
{
	BOOST_CHECK_NO_THROW(SerialAdapterDevice device(device_type, *this, false));
}

// =============================================================================
// Destruction
// =============================================================================

BOOST_AUTO_TEST_CASE(TestDestruction_CleanAfterConstruction)
{
	{
		SerialAdapterDevice device(device_type, *this, true);
	}
	BOOST_CHECK(true);
}

// =============================================================================
// QueueCommand
// =============================================================================

BOOST_AUTO_TEST_CASE(TestQueueCommand_DoesNotThrow)
{
	SerialAdapterDevice device(device_type, *this, true);
	BOOST_CHECK_NO_THROW(device.QueueCommand(0x05, 0x0A));
}

BOOST_AUTO_TEST_CASE(TestQueueCommand_MultipleCommands)
{
	SerialAdapterDevice device(device_type, *this, true);
	BOOST_CHECK_NO_THROW(device.QueueCommand(0x05, 0x0A));
	BOOST_CHECK_NO_THROW(device.QueueCommand(0x80, 0x10));
}

// =============================================================================
// QueuePumpCommand
// =============================================================================

BOOST_AUTO_TEST_CASE(TestQueuePumpCommand_DoesNotThrow)
{
	SerialAdapterDevice device(device_type, *this, true);
	BOOST_CHECK_NO_THROW(device.QueuePumpCommand(SerialAdapter_SystemPumpCommands::PUMP, SerialAdapter_CommandTypes::Toggle));
}

BOOST_AUTO_TEST_CASE(TestQueuePumpCommand_SetOn)
{
	SerialAdapterDevice device(device_type, *this, true);
	BOOST_CHECK_NO_THROW(device.QueuePumpCommand(SerialAdapter_SystemPumpCommands::PUMP, SerialAdapter_CommandTypes::SetOn));
}

BOOST_AUTO_TEST_CASE(TestQueuePumpCommand_SetOff)
{
	SerialAdapterDevice device(device_type, *this, true);
	BOOST_CHECK_NO_THROW(device.QueuePumpCommand(SerialAdapter_SystemPumpCommands::SPA, SerialAdapter_CommandTypes::SetOff));
}

// =============================================================================
// QueueAuxCommand
// =============================================================================

BOOST_AUTO_TEST_CASE(TestQueueAuxCommand_DoesNotThrow)
{
	SerialAdapterDevice device(device_type, *this, true);
	BOOST_CHECK_NO_THROW(device.QueueAuxCommand(AqualinkAutomate::Auxillaries::JandyAuxillaryIds::Aux_1, SerialAdapter_CommandTypes::Toggle));
}

BOOST_AUTO_TEST_CASE(TestQueueAuxCommand_SetOn)
{
	SerialAdapterDevice device(device_type, *this, true);
	BOOST_CHECK_NO_THROW(device.QueueAuxCommand(AqualinkAutomate::Auxillaries::JandyAuxillaryIds::Aux_3, SerialAdapter_CommandTypes::SetOn));
}

// =============================================================================
// QueueSetpointCommand
// =============================================================================

BOOST_AUTO_TEST_CASE(TestQueueSetpointCommand_DoesNotThrow)
{
	SerialAdapterDevice device(device_type, *this, true);
	BOOST_CHECK_NO_THROW(device.QueueSetpointCommand(SerialAdapter_SystemTemperatureCommands::POOLSP, 82));
}

BOOST_AUTO_TEST_CASE(TestQueueSetpointCommand_SpaSetpoint)
{
	SerialAdapterDevice device(device_type, *this, true);
	BOOST_CHECK_NO_THROW(device.QueueSetpointCommand(SerialAdapter_SystemTemperatureCommands::SPASP, 100));
}

// =============================================================================
// Command sequencing
// =============================================================================

BOOST_AUTO_TEST_CASE(TestCommandSequence_PumpThenAux)
{
	SerialAdapterDevice device(device_type, *this, true);
	device.QueuePumpCommand(SerialAdapter_SystemPumpCommands::PUMP, SerialAdapter_CommandTypes::SetOn);
	BOOST_CHECK_NO_THROW(device.QueueAuxCommand(AqualinkAutomate::Auxillaries::JandyAuxillaryIds::Aux_1, SerialAdapter_CommandTypes::SetOn));
}

// =============================================================================
// Destruction after queuing
// =============================================================================

BOOST_AUTO_TEST_CASE(TestDestruction_AfterQueuing)
{
	{
		SerialAdapterDevice device(device_type, *this, true);
		device.QueueCommand(0x05, 0x0A);
		device.QueuePumpCommand(SerialAdapter_SystemPumpCommands::PUMP, SerialAdapter_CommandTypes::Toggle);
		device.QueueAuxCommand(AqualinkAutomate::Auxillaries::JandyAuxillaryIds::Aux_2, SerialAdapter_CommandTypes::SetOff);
		device.QueueSetpointCommand(SerialAdapter_SystemTemperatureCommands::POOLSP, 82);
	}
	BOOST_CHECK(true);
}

// =============================================================================
// Different device ID
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConstruction_DifferentDeviceId)
{
	auto device_type_49 = std::make_shared<JandyDeviceType>(JandyDeviceId(0x49));
	BOOST_CHECK_NO_THROW(SerialAdapterDevice device(device_type_49, *this, true));
}

BOOST_AUTO_TEST_SUITE_END()
