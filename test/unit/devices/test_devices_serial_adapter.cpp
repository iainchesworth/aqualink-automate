#include <cstdint>
#include <memory>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/serial_adapter_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/messages/serial_adapter/serial_adapter_message_dev_status.h"
#include "jandy/auxillaries/jandy_auxillary_id.h"
#include "jandy/auxillaries/jandy_auxillary_traits_types.h"
#include "jandy/devices/capabilities/actuation_types.h"
#include "kernel/circulation.h"
#include "kernel/auxillary_devices/auxillary_device.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"

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
// SetCirculationMode (OBS-06)
// =============================================================================

BOOST_AUTO_TEST_CASE(TestSetCirculationMode_PoolSpaSpillover_Accepted)
{
	SerialAdapterDevice device(device_type, *this, true);
	BOOST_CHECK(device.SetCirculationMode(AqualinkAutomate::Kernel::CirculationModes::Pool) == Capabilities::ActuationResult::Accepted);
	BOOST_CHECK(device.SetCirculationMode(AqualinkAutomate::Kernel::CirculationModes::Spa) == Capabilities::ActuationResult::Accepted);
	BOOST_CHECK(device.SetCirculationMode(AqualinkAutomate::Kernel::CirculationModes::Spillover) == Capabilities::ActuationResult::Accepted);
}

BOOST_AUTO_TEST_CASE(TestSetCirculationMode_SpaFillDrain_NotSupported)
{
	// Spa Fill / Spa Drain have no RS-485 command (valve/service operations) -> NotSupported,
	// not InvalidValue or a silent accept.
	SerialAdapterDevice device(device_type, *this, true);
	BOOST_CHECK(device.SetCirculationMode(AqualinkAutomate::Kernel::CirculationModes::SpaFill) == Capabilities::ActuationResult::NotSupported);
	BOOST_CHECK(device.SetCirculationMode(AqualinkAutomate::Kernel::CirculationModes::SpaDrain) == Capabilities::ActuationResult::NotSupported);
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
// ActuateDevice deliverability gate (regression: auto-mode "click does nothing")
//
// An emulated adapter only transmits in response to a master poll; every poll
// Kick()s the watchdog, so IsRunning() reflects whether the master is actively
// polling our address. When the master never polls 0x48 (the RS Serial Adapter is
// an optional add-on), IsRunning() falls false and a queued command would never be
// transmitted. ActuateDevice must therefore report NotSupported (so the dispatcher
// falls back to a controller the master IS polling, e.g. an emulated OneTouch), NOT
// a false Accepted. (The positive end-to-end path -- a real setDev {state, devID}
// frame on the wire -- is covered in test_flow_command_to_wire.cpp.)
// =============================================================================

namespace
{
	// Test shim: exposes the protected watchdog Stop() so a test can simulate "the master
	// is not polling this adapter" (IsRunning() == false) without waiting out the real 30s
	// watchdog timeout.
	struct StoppableSerialAdapterDevice : public SerialAdapterDevice
	{
		using SerialAdapterDevice::SerialAdapterDevice;
		void SimulateNotPolled() { Stop(); }
	};
}

BOOST_AUTO_TEST_CASE(TestActuateDevice_NotRunning_ReturnsNotSupported)
{
	using namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes;

	// An otherwise fully-actuatable hardware aux (label + type + aux id).
	auto aux = std::make_shared<AqualinkAutomate::Kernel::AuxillaryDevice>();
	aux->AuxillaryTraits.Set(LabelTrait{}, std::string{ "Pool Light" });
	aux->AuxillaryTraits.Set(AuxillaryTypeTrait{}, AuxillaryTypes::Auxillary);
	aux->AuxillaryTraits.Set(AqualinkAutomate::Auxillaries::JandyAuxillaryId{}, AqualinkAutomate::Auxillaries::JandyAuxillaryIds::Aux_5);

	// Emulated and actively polled (running from construction): the gate passes and the
	// command is accepted -- proving the aux IS mappable, so it is the gate (not a mapping
	// failure) that blocks the not-running case below.
	StoppableSerialAdapterDevice running(device_type, *this, true);
	BOOST_CHECK(running.ActuateDevice(aux, Capabilities::ActuationAction::On) == Capabilities::ActuationResult::Accepted);

	// Emulated but the master is NOT polling our address (watchdog stopped): the queued
	// command could never be transmitted, so ActuateDevice must report NotSupported, never
	// a false Accepted.
	StoppableSerialAdapterDevice not_polled(device_type, *this, true);
	not_polled.SimulateNotPolled();
	BOOST_CHECK(not_polled.ActuateDevice(aux, Capabilities::ActuationAction::On) == Capabilities::ActuationResult::NotSupported);
	BOOST_CHECK(not_polled.ActuateDevice(aux, Capabilities::ActuationAction::Off) == Capabilities::ActuationResult::NotSupported);
	BOOST_CHECK(not_polled.ActuateDevice(aux, Capabilities::ActuationAction::Toggle) == Capabilities::ActuationResult::NotSupported);
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

// =============================================================================
// Presence gating: SuppressEmulation latch on the Emulated capability
// =============================================================================

BOOST_AUTO_TEST_CASE(TestPresenceGating_EmulatedDeviceStartsActiveNotSuppressed)
{
	SerialAdapterDevice device(device_type, *this, true);
	BOOST_CHECK(device.IsEmulated());
	BOOST_CHECK(!device.IsEmulationSuppressed());
	BOOST_CHECK(device.IsEmulationActive());
}

BOOST_AUTO_TEST_CASE(TestPresenceGating_SuppressEmulationIsOneWayLatch)
{
	SerialAdapterDevice device(device_type, *this, true);

	device.SuppressEmulation();
	BOOST_CHECK(device.IsEmulated());          // construction intent is unchanged
	BOOST_CHECK(device.IsEmulationSuppressed());
	BOOST_CHECK(!device.IsEmulationActive());

	// Idempotent: calling again keeps it suppressed.
	device.SuppressEmulation();
	BOOST_CHECK(device.IsEmulationSuppressed());
	BOOST_CHECK(!device.IsEmulationActive());
}

BOOST_AUTO_TEST_CASE(TestPresenceGating_NonEmulatedDeviceIsNeverActive)
{
	SerialAdapterDevice device(device_type, *this, false);
	BOOST_CHECK(!device.IsEmulated());
	BOOST_CHECK(!device.IsEmulationActive());
}

// =============================================================================
// Capture-gated write methods (no-throw queuing)
// =============================================================================

BOOST_AUTO_TEST_CASE(TestQueueSetpointWriteTwoStep_DoesNotThrow)
{
	SerialAdapterDevice device(device_type, *this, true);
	BOOST_CHECK_NO_THROW(device.QueueSetpointWrite_TwoStep(SerialAdapter_SystemTemperatureCommands::POOLSP, 82));
}

BOOST_AUTO_TEST_CASE(TestQueueAuxToggleWrite_DoesNotThrow)
{
	SerialAdapterDevice device(device_type, *this, true);
	BOOST_CHECK_NO_THROW(device.QueueAuxToggleWrite(AqualinkAutomate::Auxillaries::JandyAuxillaryIds::Aux_1, true));
	BOOST_CHECK_NO_THROW(device.QueueAuxToggleWrite(AqualinkAutomate::Auxillaries::JandyAuxillaryIds::Aux_2, false));
}

BOOST_AUTO_TEST_SUITE_END()
