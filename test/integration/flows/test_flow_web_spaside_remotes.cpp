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

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_SUITE_END()
