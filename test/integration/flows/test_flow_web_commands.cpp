#include <boost/test/unit_test.hpp>

#include <string>

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include "http/webroute_equipment.h"
#include "http/webroute_equipment_setpoints.h"
#include "http/server/routing/routing.h"

#include "support/integration_test_pipeline.h"
#include "support/integration_test_scenarios.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Kernel;
using namespace AqualinkAutomate::Test;

//=============================================================================
// Web command → DataHub flow tests
//
// These tests verify that HTTP endpoints correctly serve equipment data
// and that the setpoint endpoints are properly wired.
// Full POST → CommandDispatcher → serial tests require a SerialAdapterDevice,
// so we focus on GET route data integrity across equipment configurations.
//=============================================================================

BOOST_FIXTURE_TEST_SUITE(TestSuite_Integration_Flow_WebCommands, IntegrationPipeline)

BOOST_AUTO_TEST_CASE(Test_Flow_Web_SetpointRouteRegisters)
{
	HTTP::Routing::Clear();

	auto route = std::make_unique<HTTP::WebRoute_Equipment_Setpoints>(m_HubLocator);
	auto route_url = route->Route();
	HTTP::Routing::Add(std::move(route));

	BOOST_CHECK_EQUAL(route_url, "/api/equipment/setpoints");

	// GET should return current setpoints
	Scenarios::PopulatePoolOnly(*DataHub());

	auto resp = PerformHttpGet(route_url);
	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());

	auto json = nlohmann::json::parse(resp.body());
	BOOST_CHECK(json.contains("pool_setpoint") || json.contains("spa_setpoint"));

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Flow_Web_DualBodySetpoints)
{
	HTTP::Routing::Clear();

	auto route = std::make_unique<HTTP::WebRoute_Equipment_Setpoints>(m_HubLocator);
	auto route_url = route->Route();
	HTTP::Routing::Add(std::move(route));

	Scenarios::PopulateDualBodyShared(*DataHub());

	auto resp = PerformHttpGet(route_url);
	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());

	auto json = nlohmann::json::parse(resp.body());

	// Dual body should have both pool and spa setpoints
	BOOST_CHECK(json.contains("pool_setpoint"));
	BOOST_CHECK(json.contains("spa_setpoint"));

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Flow_Web_EquipmentRouteDeviceCounts_PoolOnly)
{
	HTTP::Routing::Clear();

	auto route = std::make_unique<HTTP::WebRoute_Equipment>(m_HubLocator);
	auto route_url = route->Route();
	HTTP::Routing::Add(std::move(route));

	Scenarios::PopulatePoolOnly(*DataHub());

	auto resp = PerformHttpGet(route_url);
	auto json = nlohmann::json::parse(resp.body());

	BOOST_REQUIRE(json.contains("devices"));
	BOOST_CHECK_EQUAL(json["devices"]["auxillaries"].size(), 2);
	BOOST_CHECK_EQUAL(json["devices"]["heaters"].size(), 1);
	BOOST_CHECK_EQUAL(json["devices"]["pumps"].size(), 1);

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Flow_Web_EquipmentRouteDeviceCounts_DualBodyDual)
{
	HTTP::Routing::Clear();

	auto route = std::make_unique<HTTP::WebRoute_Equipment>(m_HubLocator);
	auto route_url = route->Route();
	HTTP::Routing::Add(std::move(route));

	Scenarios::PopulateDualBodyDual(*DataHub());

	auto resp = PerformHttpGet(route_url);
	auto json = nlohmann::json::parse(resp.body());

	BOOST_REQUIRE(json.contains("devices"));
	BOOST_CHECK_EQUAL(json["devices"]["auxillaries"].size(), 2);
	BOOST_CHECK_EQUAL(json["devices"]["heaters"].size(), 2);
	BOOST_CHECK_EQUAL(json["devices"]["pumps"].size(), 2);

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_SUITE_END()
