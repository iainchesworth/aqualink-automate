#include <memory>
#include <string>

#include <boost/test/unit_test.hpp>

#include "jandy/devices/command_dispatcher.h"
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

BOOST_AUTO_TEST_SUITE_END()
