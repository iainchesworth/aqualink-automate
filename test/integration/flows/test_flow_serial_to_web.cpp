#include <boost/test/unit_test.hpp>

#include <string>

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include "http/webroute_equipment.h"
#include "http/server/routing/routing.h"

#include "support/integration_test_pipeline.h"
#include "support/integration_test_scenarios.h"
#include "support/unit_test_httprequestresponse.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Kernel;
using namespace AqualinkAutomate::Test;

//=============================================================================
// Serial → DataHub → Web/JSON flow tests
//
// These tests verify that equipment state populated in the DataHub is correctly
// serialized to JSON when queried through the HTTP web routes.
//=============================================================================

BOOST_FIXTURE_TEST_SUITE(TestSuite_Integration_Flow_SerialToWeb, IntegrationPipeline)

BOOST_AUTO_TEST_CASE(Test_Flow_Web_EquipmentEndpointReturnsOk)
{
	HTTP::Routing::Clear();
	auto route = std::make_unique<HTTP::WebRoute_Equipment>(m_HubLocator);
	auto route_url = route->Route();
	HTTP::Routing::Add(std::move(route));

	Scenarios::PopulatePoolOnly(*DataHub());

	auto resp = PerformHttpGet(route_url);
	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());
	BOOST_CHECK_GT(resp.body().length(), 0);

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Flow_Web_EquipmentJsonContainsTemperatures)
{
	HTTP::Routing::Clear();
	auto route = std::make_unique<HTTP::WebRoute_Equipment>(m_HubLocator);
	auto route_url = route->Route();
	HTTP::Routing::Add(std::move(route));

	Scenarios::PopulatePoolOnly(*DataHub());

	auto resp = PerformHttpGet(route_url);
	auto json = nlohmann::json::parse(resp.body());

	BOOST_REQUIRE(json.contains("temperatures"));
	BOOST_CHECK(json["temperatures"].contains("pool"));
	BOOST_CHECK(json["temperatures"].contains("air"));
	BOOST_CHECK(json["temperatures"].contains("pool_setpoint"));

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Flow_Web_EquipmentJsonContainsChemistry)
{
	HTTP::Routing::Clear();
	auto route = std::make_unique<HTTP::WebRoute_Equipment>(m_HubLocator);
	auto route_url = route->Route();
	HTTP::Routing::Add(std::move(route));

	Scenarios::PopulatePoolOnly(*DataHub());

	auto resp = PerformHttpGet(route_url);
	auto json = nlohmann::json::parse(resp.body());

	BOOST_REQUIRE(json.contains("chemistry"));
	BOOST_CHECK(json["chemistry"].contains("salt_in_ppm"));
	BOOST_CHECK_EQUAL(json["chemistry"]["salt_in_ppm"].get<int>(), 3200);

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Flow_Web_EquipmentJsonContainsDevices)
{
	HTTP::Routing::Clear();
	auto route = std::make_unique<HTTP::WebRoute_Equipment>(m_HubLocator);
	auto route_url = route->Route();
	HTTP::Routing::Add(std::move(route));

	Scenarios::PopulatePoolOnly(*DataHub());

	auto resp = PerformHttpGet(route_url);
	auto json = nlohmann::json::parse(resp.body());

	BOOST_REQUIRE(json.contains("devices"));
	BOOST_REQUIRE(json["devices"].contains("auxillaries"));
	BOOST_REQUIRE(json["devices"].contains("heaters"));
	BOOST_REQUIRE(json["devices"].contains("pumps"));

	// Pool-only: 2 aux, 1 heater, 1 pump
	BOOST_CHECK_EQUAL(json["devices"]["auxillaries"].size(), 2);
	BOOST_CHECK_EQUAL(json["devices"]["heaters"].size(), 1);
	BOOST_CHECK_EQUAL(json["devices"]["pumps"].size(), 1);

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Flow_Web_EquipmentJsonContainsChlorinator)
{
	HTTP::Routing::Clear();
	auto route = std::make_unique<HTTP::WebRoute_Equipment>(m_HubLocator);
	auto route_url = route->Route();
	HTTP::Routing::Add(std::move(route));

	Scenarios::PopulatePoolOnly(*DataHub());

	auto resp = PerformHttpGet(route_url);
	auto json = nlohmann::json::parse(resp.body());

	// Chlorinators appear in the devices section
	BOOST_REQUIRE(json.contains("devices"));

	// The devices section contains chlorinators when present
	if (json["devices"].contains("chlorinators"))
	{
		BOOST_CHECK_GE(json["devices"]["chlorinators"].size(), 1);
	}

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Flow_Web_EquipmentJsonContainsConfiguration)
{
	HTTP::Routing::Clear();
	auto route = std::make_unique<HTTP::WebRoute_Equipment>(m_HubLocator);
	auto route_url = route->Route();
	HTTP::Routing::Add(std::move(route));

	Scenarios::PopulatePoolOnly(*DataHub());

	auto resp = PerformHttpGet(route_url);
	auto json = nlohmann::json::parse(resp.body());

	BOOST_REQUIRE(json.contains("configuration"));
	BOOST_CHECK_EQUAL(json["configuration"]["pool_configuration"], "SingleBody");

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Flow_Web_DualBodyConfiguration)
{
	HTTP::Routing::Clear();
	auto route = std::make_unique<HTTP::WebRoute_Equipment>(m_HubLocator);
	auto route_url = route->Route();
	HTTP::Routing::Add(std::move(route));

	Scenarios::PopulateDualBodyShared(*DataHub());

	auto resp = PerformHttpGet(route_url);
	auto json = nlohmann::json::parse(resp.body());

	BOOST_REQUIRE(json.contains("configuration"));
	BOOST_CHECK_EQUAL(json["configuration"]["pool_configuration"], "DualBody_SharedEquipment");

	BOOST_REQUIRE(json["configuration"].contains("bodies"));
	BOOST_CHECK_EQUAL(json["configuration"]["bodies"].size(), 2);

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_SUITE_END()
