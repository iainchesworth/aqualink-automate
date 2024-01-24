#include <boost/test/unit_test.hpp>

#include "http/webroute_page_index.h"
#include "http/webroute_page_equipment.h"
#include "http/webroute_page_version.h"
#include "http/websocket_equipment.h"
#include "http/websocket_equipment_stats.h"
#include "http/server/routing/routing.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"

#include "support/unit_test_httprequestresponse.h"

using namespace AqualinkAutomate;

BOOST_AUTO_TEST_SUITE(TestSuite_HttpSessions)

BOOST_AUTO_TEST_CASE(Test_HttpSessions_PlainSession_RenderedPages)
{
	boost::asio::io_context io_context;

	Kernel::HubLocator hub_locator;
	hub_locator.Register(std::make_shared<Kernel::DataHub>());

	HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Page_Index>(hub_locator));
	HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Page_Equipment>(hub_locator));
	HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Page_Version>());

	/// TEST API ROUTE: /

	{
		auto&& tro = Test::PerformHttpRequestResponse(io_context, "/");
		BOOST_CHECK_EQUAL(boost::beast::http::status::ok, tro->last_response.result());

		const auto& res_body = tro->last_response.body();
		BOOST_TEST_REQUIRE(0 != res_body.length());
	}

	/// TEST API ROUTE: /equipment

	{
		auto&& tro = Test::PerformHttpRequestResponse(io_context, "/equipment");
		BOOST_CHECK_EQUAL(boost::beast::http::status::ok, tro->last_response.result());

		const auto& res_body = tro->last_response.body();
		BOOST_TEST_REQUIRE(0 != res_body.length());
	}

	/// TEST API ROUTE: /version

	{
		auto&& tro = Test::PerformHttpRequestResponse(io_context, "/version");
		BOOST_CHECK_EQUAL(boost::beast::http::status::ok, tro->last_response.result());

		const auto& res_body = tro->last_response.body();
		BOOST_TEST_REQUIRE(0 != res_body.length());
	}
}

BOOST_AUTO_TEST_CASE(Test_HttpSessions_PlainWebSocket_API)
{
	boost::asio::io_context io_context;

	Kernel::HubLocator hub_locator;
	hub_locator
		.Register(std::make_shared<Kernel::DataHub>())
		.Register(std::make_shared<Kernel::EquipmentHub>())
		.Register(std::make_shared<Kernel::StatisticsHub>());

	HTTP::Routing::Add(std::make_unique<HTTP::WebSocket_Equipment>(hub_locator));
	HTTP::Routing::Add(std::make_unique<HTTP::WebSocket_Equipment_Stats>(hub_locator));

	/// TEST WEBSOCKET ROUTE: /ws/equipment/stats

	{
		auto&& tro = Test::PerformHttpWsUpgradeResponse(io_context, "/ws/equipment/stats");
	}
}

BOOST_AUTO_TEST_SUITE_END()
