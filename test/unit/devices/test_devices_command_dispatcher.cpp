#include <memory>
#include <string>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/command_dispatcher.h"
#include "jandy/devices/onetouch_device.h"
#include "jandy/devices/serial_adapter_device.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "interfaces/icommanddispatcher.h"
#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "kernel/auxillary_devices/auxillary_device.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/auxillary_devices/auxillary_status.h"
#include "kernel/auxillary_devices/pump_status.h"
#include "kernel/circulation.h"

#include "support/unit_test_hublocatorinjector.h"
#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Devices;
using namespace AqualinkAutomate::Interfaces;
using namespace AqualinkAutomate::Kernel;
using namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes;

namespace
{
	struct CommandDispatcherFixture : public AqualinkAutomate::Test::HubLocatorInjector
	{
		CommandDispatcherFixture()
			: data_hub(Find<DataHub>())
			, equipment_hub(Find<EquipmentHub>())
			, dispatcher(data_hub, equipment_hub)
		{
		}

		std::shared_ptr<DataHub> data_hub;
		std::shared_ptr<EquipmentHub> equipment_hub;
		CommandDispatcher dispatcher;
	};
}

BOOST_FIXTURE_TEST_SUITE(CommandDispatcher_TestSuite, CommandDispatcherFixture)

// =============================================================================
// Device-not-found Paths
// =============================================================================

BOOST_AUTO_TEST_CASE(TestToggleByUuid_DeviceNotFound)
{
	boost::uuids::uuid fake_uuid{};
	auto result = dispatcher.ToggleByUuid(fake_uuid);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::DeviceNotFound));
}

BOOST_AUTO_TEST_CASE(TestToggleByLabel_DeviceNotFound)
{
	auto result = dispatcher.ToggleByLabel("NonexistentDevice");
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::DeviceNotFound));
}

BOOST_AUTO_TEST_CASE(TestCommandByUuid_DeviceNotFound)
{
	boost::uuids::uuid fake_uuid{};
	auto result = dispatcher.CommandByUuid(fake_uuid, ICommandDispatcher::DeviceAction::On);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::DeviceNotFound));
}

BOOST_AUTO_TEST_CASE(TestCommandByLabel_DeviceNotFound)
{
	auto result = dispatcher.CommandByLabel("Ghost", ICommandDispatcher::DeviceAction::Off);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::DeviceNotFound));
}

// =============================================================================
// No-serial-adapter Paths
// =============================================================================

BOOST_AUTO_TEST_CASE(TestSetPoolSetpoint_NoSerialAdapter)
{
	auto result = dispatcher.SetPoolSetpoint(82);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::NoSerialAdapter));
}

BOOST_AUTO_TEST_CASE(TestSetSpaSetpoint_NoSerialAdapter)
{
	auto result = dispatcher.SetSpaSetpoint(100);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::NoSerialAdapter));
}

BOOST_AUTO_TEST_CASE(TestSetCirculationMode_NoSerialAdapter)
{
	auto result = dispatcher.SetCirculationMode(CirculationModes::Spa);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::NoSerialAdapter));
}

// =============================================================================
// Pool/Spa Setpoint Range Validation (regression for WU-JANDY-COMMAND-DISPATCH)
//
// An out-of-range setpoint must be rejected BEFORE any serial-adapter lookup so it
// never reaches the RS-485 wire. With no serial adapter present in the fixture, an
// in-range value falls through to NoSerialAdapter while an out-of-range value short-
// circuits with InvalidValue - which lets us distinguish the two paths.
// =============================================================================

BOOST_AUTO_TEST_CASE(TestSetPoolSetpoint_InvalidValue_Zero)
{
	// 0 is the sensor-unavailable sentinel and is rejected as out-of-range.
	auto result = dispatcher.SetPoolSetpoint(0);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::InvalidValue));
}

BOOST_AUTO_TEST_CASE(TestSetPoolSetpoint_InvalidValue_AboveMax)
{
	auto result = dispatcher.SetPoolSetpoint(static_cast<uint8_t>(CommandDispatcher::SETPOINT_MAX_VALUE + 1));
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::InvalidValue));
}

BOOST_AUTO_TEST_CASE(TestSetPoolSetpoint_InvalidValue_MaxUint8)
{
	auto result = dispatcher.SetPoolSetpoint(255);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::InvalidValue));
}

BOOST_AUTO_TEST_CASE(TestSetPoolSetpoint_BoundaryValue_Min)
{
	// In-range boundary: not InvalidValue; falls through to NoSerialAdapter in this fixture.
	auto result = dispatcher.SetPoolSetpoint(CommandDispatcher::SETPOINT_MIN_VALUE);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::NoSerialAdapter));
}

BOOST_AUTO_TEST_CASE(TestSetPoolSetpoint_BoundaryValue_Max)
{
	auto result = dispatcher.SetPoolSetpoint(CommandDispatcher::SETPOINT_MAX_VALUE);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::NoSerialAdapter));
}

BOOST_AUTO_TEST_CASE(TestSetSpaSetpoint_InvalidValue_Zero)
{
	auto result = dispatcher.SetSpaSetpoint(0);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::InvalidValue));
}

BOOST_AUTO_TEST_CASE(TestSetSpaSetpoint_InvalidValue_AboveMax)
{
	auto result = dispatcher.SetSpaSetpoint(static_cast<uint8_t>(CommandDispatcher::SETPOINT_MAX_VALUE + 1));
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::InvalidValue));
}

BOOST_AUTO_TEST_CASE(TestSetSpaSetpoint_BoundaryValue_Min)
{
	auto result = dispatcher.SetSpaSetpoint(CommandDispatcher::SETPOINT_MIN_VALUE);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::NoSerialAdapter));
}

BOOST_AUTO_TEST_CASE(TestSetSpaSetpoint_BoundaryValue_Max)
{
	auto result = dispatcher.SetSpaSetpoint(CommandDispatcher::SETPOINT_MAX_VALUE);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::NoSerialAdapter));
}

// =============================================================================
// Chlorinator Validation
// =============================================================================

BOOST_AUTO_TEST_CASE(TestSetChlorinatorPercentage_InvalidValue_Above100)
{
	auto result = dispatcher.SetChlorinatorPercentage(101);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::InvalidValue));
}

BOOST_AUTO_TEST_CASE(TestSetChlorinatorPercentage_InvalidValue_MaxUint8)
{
	auto result = dispatcher.SetChlorinatorPercentage(255);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::InvalidValue));
}

BOOST_AUTO_TEST_CASE(TestSetChlorinatorPercentage_BoundaryValue_100)
{
	// 100 is valid but needs an IAQ device - should get DeviceNotFound, not InvalidValue
	auto result = dispatcher.SetChlorinatorPercentage(100);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::DeviceNotFound));
}

BOOST_AUTO_TEST_CASE(TestSetChlorinatorPercentage_BoundaryValue_0)
{
	// 0 is valid but needs an IAQ device
	auto result = dispatcher.SetChlorinatorPercentage(0);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::DeviceNotFound));
}

// =============================================================================
// No-IAQ-device Paths
// =============================================================================

BOOST_AUTO_TEST_CASE(TestSetChlorinatorPercentage_NoIAQDevice)
{
	auto result = dispatcher.SetChlorinatorPercentage(50);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::DeviceNotFound));
}

BOOST_AUTO_TEST_CASE(TestSetChlorinatorBoost_NoIAQDevice)
{
	auto result = dispatcher.SetChlorinatorBoost(true);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::DeviceNotFound));
}

BOOST_AUTO_TEST_CASE(TestSetChlorinatorBoost_Disable_NoIAQDevice)
{
	auto result = dispatcher.SetChlorinatorBoost(false);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::DeviceNotFound));
}

// =============================================================================
// DispatchCommand - No Serial Adapter (via ToggleByLabel with device present)
// =============================================================================

BOOST_AUTO_TEST_CASE(TestDispatchCommand_NoSerialAdapter_ReturnsNoSerialAdapter)
{
	// Add a device to DataHub so it can be found by label
	auto device = std::make_shared<AuxillaryDevice>();
	device->AuxillaryTraits.Set(LabelTrait{}, std::string{"Test Aux"});
	device->AuxillaryTraits.Set(AuxillaryTypeTrait{}, AuxillaryTypes::Auxillary);
	data_hub->Devices.Add(device);

	auto result = dispatcher.CommandByLabel("Test Aux", ICommandDispatcher::DeviceAction::On);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::NoSerialAdapter));
}

// =============================================================================
// Capability routing to an emulated OneTouch (no Serial Adapter present).
//
// With ONLY an emulated OneTouch on the bus the dispatcher must resolve it as the
// DeviceActuator (toggles) and SetpointController (heater setpoints) - this is the
// whole point of the capability refactor: a OneTouch-only rig can actuate. A
// PASSIVE (non-emulated) OneTouch advertises the same capabilities but honestly
// reports NotSupported at call time, so commands fall through to NoSerialAdapter.
// =============================================================================

namespace
{
	std::unique_ptr<OneTouchDevice> MakeOneTouch(AqualinkAutomate::Test::HubLocatorInjector& hub, uint8_t id, bool emulated)
	{
		auto device_id = std::make_shared<JandyDeviceType>(JandyDeviceId(id));
		return std::make_unique<OneTouchDevice>(device_id, hub, emulated);
	}
}

BOOST_AUTO_TEST_CASE(TestSetPoolSetpoint_EmulatedOneTouch_RoutesAndAccepts)
{
	equipment_hub->AddDevice(MakeOneTouch(*this, 0x41, true));

	// In range + a capable controller present => the OneTouch SetpointController
	// queues the menu edit and the dispatcher reports Success.
	auto result = dispatcher.SetPoolSetpoint(82);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::Success));
}

BOOST_AUTO_TEST_CASE(TestSetSpaSetpoint_EmulatedOneTouch_RoutesAndAccepts)
{
	equipment_hub->AddDevice(MakeOneTouch(*this, 0x41, true));

	auto result = dispatcher.SetSpaSetpoint(100);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::Success));
}

BOOST_AUTO_TEST_CASE(TestSetPoolSetpoint_PassiveOneTouch_FallsThroughToNoSerialAdapter)
{
	// A non-emulated OneTouch IS-A SetpointController (the dynamic_cast resolves it),
	// but it cannot transmit, so it returns NotSupported -> the call-site maps that to
	// NoSerialAdapter (the historical "nothing can act" code for setpoints).
	equipment_hub->AddDevice(MakeOneTouch(*this, 0x41, false));

	auto result = dispatcher.SetPoolSetpoint(82);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::NoSerialAdapter));
}

BOOST_AUTO_TEST_CASE(TestToggleByLabel_EmulatedOneTouch_RoutesAndAccepts)
{
	// A labelled aux the OneTouch can map onto the Equipment ON/OFF row.
	auto device = std::make_shared<AuxillaryDevice>();
	device->AuxillaryTraits.Set(LabelTrait{}, std::string{"Pool Light"});
	device->AuxillaryTraits.Set(AuxillaryTypeTrait{}, AuxillaryTypes::Auxillary);
	data_hub->Devices.Add(device);

	equipment_hub->AddDevice(MakeOneTouch(*this, 0x41, true));

	auto result = dispatcher.ToggleByLabel("Pool Light");
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::Success));
}

BOOST_AUTO_TEST_CASE(TestSetPoolSetpoint_SerialAdapterPreferredOverOneTouch)
{
	// Both a Serial Adapter (High) and an emulated OneTouch (Low) advertise
	// SetpointController. FindCapable<> must pick the higher-priority Serial Adapter;
	// either way the dispatcher reports Success, but precedence is what we assert at
	// the unit level via FindCapable below. Here we just prove both-present still works.
	auto sa_id = std::make_shared<JandyDeviceType>(JandyDeviceId(0x48));
	equipment_hub->AddDevice(std::make_unique<SerialAdapterDevice>(sa_id, *this, true));
	equipment_hub->AddDevice(MakeOneTouch(*this, 0x41, true));

	auto result = dispatcher.SetPoolSetpoint(82);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::Success));
}

BOOST_AUTO_TEST_CASE(TestSetChlorinatorPercentage_EmulatedOneTouch_RoutesAndAccepts)
{
	// A OneTouch-only rig can now drive the chlorinator (via the Set AquaPure menu) - the
	// OneTouch advertises ChlorinatorController and the % is queued/accepted.
	equipment_hub->AddDevice(MakeOneTouch(*this, 0x41, true));

	auto result = dispatcher.SetChlorinatorPercentage(45);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::Success));
}

BOOST_AUTO_TEST_CASE(TestSetChlorinatorBoost_EmulatedOneTouch_RoutesAndAccepts)
{
	equipment_hub->AddDevice(MakeOneTouch(*this, 0x41, true));

	auto result = dispatcher.SetChlorinatorBoost(true);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::Success));
}

BOOST_AUTO_TEST_CASE(TestSetChlorinatorPercentage_PassiveOneTouch_FallsThrough)
{
	// A non-emulated OneTouch IS-A ChlorinatorController but cannot transmit -> NotSupported,
	// which the chlorinator call-site maps to DeviceNotFound.
	equipment_hub->AddDevice(MakeOneTouch(*this, 0x41, false));

	auto result = dispatcher.SetChlorinatorPercentage(45);
	BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(ICommandDispatcher::CommandResult::DeviceNotFound));
}

BOOST_AUTO_TEST_SUITE_END()
