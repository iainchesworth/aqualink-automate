#include <boost/test/unit_test.hpp>

#include <memory>
#include <string>

#include <nlohmann/json.hpp>

#include "http/webroute_diagnostics_actualdevices.h"
#include "http/webroute_diagnostics_devices.h"
#include "kernel/equipment_hub.h"

#include "jandy/devices/jandy_device_types.h"
#include "jandy/devices/keypad_device.h"

#include "support/unit_test_hublocatorinjector.h"

using namespace AqualinkAutomate;

//
// Regression coverage for the /api/diagnostics/emulated-devices filter.
//
// The endpoint must report ONLY emulated devices. Real bus-discovered devices
// are instances of the same concrete classes (and therefore share the same
// IDescribable type), so the route must filter on emulation *state*
// (IEmulatedDevice::IsEmulated()) rather than on the describable type alone.
// The original implementation used a dynamic_cast<IDescribable*> type check
// and leaked real devices into the response.
//

BOOST_AUTO_TEST_SUITE(TestSuite_HttpDiagnosticsDevices)

BOOST_AUTO_TEST_CASE(Test_DiagnosticsDevices_OnlyEmulatedDevicesAreReported)
{
	Test::HubLocatorInjector hub_locator;

	auto equipment_hub = hub_locator.Find<Kernel::EquipmentHub>();
	BOOST_REQUIRE(nullptr != equipment_hub);

	// One emulated device and one real device of the same concrete class.
	auto emulated_id = std::make_shared<Devices::JandyDeviceType>(0x08);
	auto real_id = std::make_shared<Devices::JandyDeviceType>(0x09);

	BOOST_REQUIRE(equipment_hub->AddDevice(std::make_unique<Devices::KeypadDevice>(emulated_id, hub_locator, true)));
	BOOST_REQUIRE(equipment_hub->AddDevice(std::make_unique<Devices::KeypadDevice>(real_id, hub_locator, false)));

	HTTP::WebRoute_Diagnostics_Devices route(hub_locator);
	const auto result = route.CollectEmulatedDiagnostics();

	BOOST_REQUIRE(result.is_array());
	BOOST_REQUIRE_EQUAL(result.size(), 1U);
	BOOST_CHECK_EQUAL(result[0]["device_type"].get<std::string>(), "Keypad");
	BOOST_CHECK_EQUAL(result[0]["device_id"].get<std::string>(), "0x08");
}

BOOST_AUTO_TEST_CASE(Test_DiagnosticsDevices_RealOnlyHubReportsNothing)
{
	Test::HubLocatorInjector hub_locator;

	auto equipment_hub = hub_locator.Find<Kernel::EquipmentHub>();
	BOOST_REQUIRE(nullptr != equipment_hub);

	auto real_id = std::make_shared<Devices::JandyDeviceType>(0x09);
	BOOST_REQUIRE(equipment_hub->AddDevice(std::make_unique<Devices::KeypadDevice>(real_id, hub_locator, false)));

	HTTP::WebRoute_Diagnostics_Devices route(hub_locator);
	const auto result = route.CollectEmulatedDiagnostics();

	BOOST_REQUIRE(result.is_array());
	BOOST_CHECK_EQUAL(result.size(), 0U);
}

//
// Coverage for the sibling /api/diagnostics/actual-devices filter.
//
// The actual-devices endpoint is the mirror image of emulated-devices: it must
// report ONLY non-emulated (real, bus-discovered) devices. Both routes share
// the same collector (CollectDeviceDiagnostics) parameterised on the emulation
// predicate; these cases assert the two filters partition the same hub.
//

BOOST_AUTO_TEST_CASE(Test_DiagnosticsActualDevices_OnlyNonEmulatedDevicesAreReported)
{
	Test::HubLocatorInjector hub_locator;

	auto equipment_hub = hub_locator.Find<Kernel::EquipmentHub>();
	BOOST_REQUIRE(nullptr != equipment_hub);

	// One emulated device and one real device of the same concrete class.
	auto emulated_id = std::make_shared<Devices::JandyDeviceType>(0x08);
	auto real_id = std::make_shared<Devices::JandyDeviceType>(0x09);

	BOOST_REQUIRE(equipment_hub->AddDevice(std::make_unique<Devices::KeypadDevice>(emulated_id, hub_locator, true)));
	BOOST_REQUIRE(equipment_hub->AddDevice(std::make_unique<Devices::KeypadDevice>(real_id, hub_locator, false)));

	HTTP::WebRoute_Diagnostics_ActualDevices route(hub_locator);
	const auto result = route.CollectActualDiagnostics();

	BOOST_REQUIRE(result.is_array());
	BOOST_REQUIRE_EQUAL(result.size(), 1U);
	BOOST_CHECK_EQUAL(result[0]["device_type"].get<std::string>(), "Keypad");
	BOOST_CHECK_EQUAL(result[0]["device_id"].get<std::string>(), "0x09");
}

BOOST_AUTO_TEST_CASE(Test_DiagnosticsActualDevices_EmulatedOnlyHubReportsNothing)
{
	Test::HubLocatorInjector hub_locator;

	auto equipment_hub = hub_locator.Find<Kernel::EquipmentHub>();
	BOOST_REQUIRE(nullptr != equipment_hub);

	auto emulated_id = std::make_shared<Devices::JandyDeviceType>(0x08);
	BOOST_REQUIRE(equipment_hub->AddDevice(std::make_unique<Devices::KeypadDevice>(emulated_id, hub_locator, true)));

	HTTP::WebRoute_Diagnostics_ActualDevices route(hub_locator);
	const auto result = route.CollectActualDiagnostics();

	BOOST_REQUIRE(result.is_array());
	BOOST_CHECK_EQUAL(result.size(), 0U);
}

BOOST_AUTO_TEST_CASE(Test_DiagnosticsDevices_EmulatedAndActualRoutesPartitionTheHub)
{
	Test::HubLocatorInjector hub_locator;

	auto equipment_hub = hub_locator.Find<Kernel::EquipmentHub>();
	BOOST_REQUIRE(nullptr != equipment_hub);

	// Two emulated and one real device of the same concrete class.
	auto emulated_id_a = std::make_shared<Devices::JandyDeviceType>(0x08);
	auto emulated_id_b = std::make_shared<Devices::JandyDeviceType>(0x0a);
	auto real_id = std::make_shared<Devices::JandyDeviceType>(0x09);

	BOOST_REQUIRE(equipment_hub->AddDevice(std::make_unique<Devices::KeypadDevice>(emulated_id_a, hub_locator, true)));
	BOOST_REQUIRE(equipment_hub->AddDevice(std::make_unique<Devices::KeypadDevice>(emulated_id_b, hub_locator, true)));
	BOOST_REQUIRE(equipment_hub->AddDevice(std::make_unique<Devices::KeypadDevice>(real_id, hub_locator, false)));

	HTTP::WebRoute_Diagnostics_Devices emulated_route(hub_locator);
	HTTP::WebRoute_Diagnostics_ActualDevices actual_route(hub_locator);

	const auto emulated_result = emulated_route.CollectEmulatedDiagnostics();
	const auto actual_result = actual_route.CollectActualDiagnostics();

	BOOST_REQUIRE(emulated_result.is_array());
	BOOST_REQUIRE(actual_result.is_array());

	// The two routes partition the hub: every device appears in exactly one.
	BOOST_CHECK_EQUAL(emulated_result.size(), 2U);
	BOOST_REQUIRE_EQUAL(actual_result.size(), 1U);
	BOOST_CHECK_EQUAL(actual_result[0]["device_id"].get<std::string>(), "0x09");
}

BOOST_AUTO_TEST_SUITE_END()
