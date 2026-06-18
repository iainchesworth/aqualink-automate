#include <boost/test/unit_test.hpp>

#include <memory>

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include "http/webroute_equipment_spaside_remotes.h"
#include "http/server/routing/routing.h"
#include "interfaces/ispasideremotecontroller.h"

#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/devices/spaside_remote_controller.h"
#include "jandy/devices/spaside_remote_device.h"

#include "support/integration_test_pipeline.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Test;

//=============================================================================
// Web read-path for spa-side remotes: GET /api/equipment/spaside-remotes ->
// SpasideRemoteController -> EquipmentHub devices -> JSON. Exercises the route,
// the controller, and the device together end-to-end.
//=============================================================================

BOOST_FIXTURE_TEST_SUITE(TestSuite_Integration_Flow_WebSpasideRemotes, IntegrationPipeline)

BOOST_AUTO_TEST_CASE(Get_NoControllerRegistered_ReturnsEmptyList)
{
	HTTP::Routing::Clear();

	// No ISpasideRemoteController registered (e.g. dev-mode/replay): the route still serves a
	// well-formed, empty list rather than failing.
	auto route = std::make_unique<HTTP::WebRoute_Equipment_SpasideRemotes>(m_HubLocator);
	auto route_url = route->Route();
	HTTP::Routing::Add(std::move(route));

	BOOST_CHECK_EQUAL(route_url, "/api/equipment/spaside-remotes");

	auto resp = PerformHttpGet(route_url);
	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());

	auto json = nlohmann::json::parse(resp.body());
	BOOST_REQUIRE(json.contains("remotes"));
	BOOST_CHECK(json["remotes"].is_array());
	BOOST_CHECK_EQUAL(json["remotes"].size(), 0u);

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Get_EmulatedSpaLink_AppearsWithDecodedFields)
{
	HTTP::Routing::Clear();

	// An emulated Spa Link at 0x20 (a class the maintainer does not own -- generality).
	auto id = std::make_shared<Devices::JandyDeviceType>(Devices::JandyDeviceId(0x20));
	EquipmentHub()->AddDevice(std::make_unique<Devices::SpasideRemoteDevice>(id, m_HubLocator, true));

	// Register the controller over that hub, then build the route (it resolves the controller).
	auto controller = std::make_shared<Devices::SpasideRemoteController>(EquipmentHub());
	m_HubLocator.Register<Interfaces::ISpasideRemoteController>(controller);

	auto route = std::make_unique<HTTP::WebRoute_Equipment_SpasideRemotes>(m_HubLocator);
	auto route_url = route->Route();
	HTTP::Routing::Add(std::move(route));

	auto resp = PerformHttpGet(route_url);
	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());

	auto json = nlohmann::json::parse(resp.body());
	BOOST_REQUIRE(json.contains("remotes"));
	BOOST_REQUIRE_EQUAL(json["remotes"].size(), 1u);

	const auto& remote = json["remotes"][0];
	BOOST_CHECK_EQUAL(remote["address"].get<std::string>(), "0x20");
	BOOST_CHECK_EQUAL(remote["device_class"].get<std::string>(), "SpaRemote");
	BOOST_CHECK_EQUAL(remote["emulated"].get<bool>(), true);
	BOOST_CHECK_EQUAL(remote["button_count"].get<int>(), 9);   // Spa Link: 8 keys + extra
	BOOST_CHECK_EQUAL(remote["led_image_seen"].get<bool>(), false);

	// Per-key buttons: a Spa Link's key->switch:button mapping is undecoded, so every key is
	// listed (pressable, since emulated) but not assignable.
	BOOST_REQUIRE(remote.contains("buttons"));
	BOOST_REQUIRE_EQUAL(remote["buttons"].size(), 9u);
	for (const auto& b : remote["buttons"])
	{
		BOOST_CHECK_EQUAL(b["assignable"].get<bool>(), false);
		BOOST_CHECK_EQUAL(b["pressable"].get<bool>(), true);
	}

	// available_functions is always present (an array); empty here -- no configurator, no in-use functions.
	BOOST_REQUIRE(json.contains("available_functions"));
	BOOST_CHECK(json["available_functions"].is_array());

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Get_EmulatedDualSpaSwitch_ButtonsCarryMappingAndLiveFunction)
{
	HTTP::Routing::Clear();

	// An emulated Dual Spa Switch (6588 board) at 0x10: keys 1-4 = Switch 2, keys 5-8 = Switch 3.
	auto id = std::make_shared<Devices::JandyDeviceType>(Devices::JandyDeviceId(0x10));
	EquipmentHub()->AddDevice(std::make_unique<Devices::SpasideRemoteDevice>(id, m_HubLocator, true));

	// A live decoded assignment on the DataHub: Switch 2 button 1 (= key index 1) -> "Pool Light".
	DataHub()->SetSpaSwitchAssignment(2, 1, "Pool Light");

	auto controller = std::make_shared<Devices::SpasideRemoteController>(EquipmentHub());
	m_HubLocator.Register<Interfaces::ISpasideRemoteController>(controller);

	auto route = std::make_unique<HTTP::WebRoute_Equipment_SpasideRemotes>(m_HubLocator);
	auto route_url = route->Route();
	HTTP::Routing::Add(std::move(route));

	auto resp = PerformHttpGet(route_url);
	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());

	auto json = nlohmann::json::parse(resp.body());
	BOOST_REQUIRE_EQUAL(json["remotes"].size(), 1u);
	const auto& remote = json["remotes"][0];
	BOOST_CHECK_EQUAL(remote["device_class"].get<std::string>(), "DualSpaSwitch");
	BOOST_REQUIRE_EQUAL(remote["buttons"].size(), 8u);

	// Key index 1 -> Switch 2 button 1, assignable, and labelled with the live function.
	const auto& key1 = remote["buttons"][0];
	BOOST_CHECK_EQUAL(key1["index"].get<int>(), 1);
	BOOST_CHECK_EQUAL(key1["switch"].get<int>(), 2);
	BOOST_CHECK_EQUAL(key1["button"].get<int>(), 1);
	BOOST_CHECK_EQUAL(key1["assignable"].get<bool>(), true);
	BOOST_CHECK_EQUAL(key1["pressable"].get<bool>(), true);
	BOOST_CHECK_EQUAL(key1["function"].get<std::string>(), "Pool Light");

	// Key index 5 -> Switch 3 button 1 (the second bridged switch), no live function yet.
	const auto& key5 = remote["buttons"][4];
	BOOST_CHECK_EQUAL(key5["switch"].get<int>(), 3);
	BOOST_CHECK_EQUAL(key5["button"].get<int>(), 1);
	BOOST_CHECK_EQUAL(key5["function"].get<std::string>(), "");

	// available_functions unions in any in-use function even with no configurator present, so a
	// strict UI chooser can never hide a currently-assigned function.
	BOOST_REQUIRE(json.contains("available_functions"));
	bool has_pool_light = false;
	for (const auto& fn : json["available_functions"])
	{
		if (fn.get<std::string>() == "Pool Light") { has_pool_light = true; }
	}
	BOOST_CHECK(has_pool_light);

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_SUITE_END()
