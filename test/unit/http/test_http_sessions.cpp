#include <boost/test/unit_test.hpp>

#include "http/webroute_page_index.h"
#include "http/webroute_page_equipment.h"
#include "http/webroute_page_version.h"
#include "http/websocket_equipment.h"
#include "http/websocket_equipment_stats.h"
#include "http/server/routing/routing.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"
#include "logging/logging.h"
#include "logging/logging_severity_filter.h"

#include "support/unit_test_httprequestresponse.h"
#include "support/unit_test_wsrequestresponse.h"

using namespace AqualinkAutomate;

BOOST_AUTO_TEST_SUITE(TestSuite_HttpSessions)

BOOST_AUTO_TEST_CASE(Test_HttpSessions_PlainSession_RenderedPages)
{
	HTTP::Routing::Clear();

	Kernel::HubLocator hub_locator;
	hub_locator.Register(std::make_shared<Kernel::DataHub>());

	HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Page_Index>(hub_locator));
	HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Page_Equipment>(hub_locator));
	HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Page_Version>());

	/// TEST API ROUTE: /

	{
		auto resp = Test::PerformHttpRequestResponse("/");
		BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());

		const auto& res_body = resp.body();
		BOOST_TEST_REQUIRE(0 != res_body.length());
	}

	/// TEST API ROUTE: /equipment

	{
		auto resp = Test::PerformHttpRequestResponse("/equipment");
		BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());

		const auto& res_body = resp.body();
		BOOST_TEST_REQUIRE(0 != res_body.length());
	}

	/// TEST API ROUTE: /version

	{
		auto resp = Test::PerformHttpRequestResponse("/version");
		BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());

		const auto& res_body = resp.body();
		BOOST_TEST_REQUIRE(0 != res_body.length());
	}
}

BOOST_AUTO_TEST_CASE(Test_HttpSessions_PlainWebSocket_API)
{
	HTTP::Routing::Clear();

	Kernel::HubLocator hub_locator;
	hub_locator
		.Register(std::make_shared<Kernel::DataHub>())
		.Register(std::make_shared<Kernel::EquipmentHub>())
		.Register(std::make_shared<Kernel::StatisticsHub>());

	HTTP::Routing::Add(std::make_unique<HTTP::WebSocket_Equipment>(hub_locator));
	HTTP::Routing::Add(std::make_unique<HTTP::WebSocket_Equipment_Stats>(hub_locator));

	/// TEST WEBSOCKET ROUTE: /ws/equipment/stats

	{
		auto [websocket_buffer, bytes_read] = Test::PerformHttpWsUpgradeResponse("/ws/equipment/stats", [&]() -> void
			{
				auto stats_hub = hub_locator.TryFind<Kernel::StatisticsHub>();
				stats_hub->MessageCounts.Signal()(1);
			}
		);
	}
}

BOOST_AUTO_TEST_CASE(Test_MultipleConcurrent_WebSockets)
{
	HTTP::Routing::Clear();

	Logging::SeverityFiltering::SetChannelFilterLevel(Channel::Web, Severity::Trace);

	boost::asio::io_context io_context;
	boost::beast::error_code ec;

	Kernel::HubLocator hub_locator;
	hub_locator
		.Register(std::make_shared<Kernel::DataHub>())
		.Register(std::make_shared<Kernel::EquipmentHub>())
		.Register(std::make_shared<Kernel::StatisticsHub>());

	HTTP::Routing::Add(std::make_unique<HTTP::WebSocket_Equipment>(hub_locator));

	/// TEST WEBSOCKET ROUTE: /ws/equipment

	{
		auto [websocket1_buffer, bytes_read1] = Test::PerformHttpWsUpgradeResponse("/ws/equipment", [&hub_locator]() -> void
			{
				throw;
			});
		auto [websocket2_buffer, bytes_read2] = Test::PerformHttpWsUpgradeResponse("/ws/equipment", [&hub_locator]() -> void
			{
				throw;
			});

		///FIXME

		/*{
			auto data_hub = hub_locator.TryFind<Kernel::DataHub>();
			BOOST_TEST_REQUIRE(nullptr != data_hub);

			data_hub->ORP(650);
			io_context.poll(); // ASYNC_READ
			io_context.poll(); // ASYNC_READ

			std::array<uint8_t, 1024> websocket1_buffer{}, websocket2_buffer{};
			tro1->local_stream.read_size(1024);
			tro2->local_stream.read_size(1024);

			auto size1 = tro1->local_stream.buffer().size();
			const auto bytes_read1 = tro1->local_stream.read_some(boost::asio::buffer(websocket1_buffer), ec);
			BOOST_TEST_REQUIRE(0 == ec.value());
			BOOST_TEST_REQUIRE(0 != bytes_read1);
			BOOST_TEST_REQUIRE(size1 == bytes_read1);

			auto size2 = tro2->local_stream.buffer().size();
			const auto bytes_read2 = tro2->local_stream.read_some(boost::asio::buffer(websocket2_buffer), ec);
			BOOST_TEST_REQUIRE(0 == ec.value());
			BOOST_TEST_REQUIRE(0 != bytes_read2);
			BOOST_TEST_REQUIRE(size2 == bytes_read2);

			nlohmann::json wse1_json, wse2_json;

			try
			{
				std::string ss1(websocket1_buffer.data() + 2, websocket1_buffer.data() + bytes_read1);
				std::string ss2(websocket2_buffer.data() + 2, websocket2_buffer.data() + bytes_read2);

				wse1_json = nlohmann::json::parse(websocket1_buffer.data() + 2, websocket1_buffer.data() + bytes_read1);
			}
			catch (const nlohmann::json::exception& e)
			{
				LogDebug(Channel::Developer, e.what());
			}

			BOOST_REQUIRE_NO_THROW(wse1_json = nlohmann::json::parse(websocket1_buffer.data() + 2, websocket1_buffer.data() + bytes_read1));
			BOOST_REQUIRE_NO_THROW(wse2_json = nlohmann::json::parse(websocket2_buffer.data() + 2, websocket2_buffer.data() + bytes_read2));

			BOOST_CHECK_EQUAL(bytes_read1, bytes_read2);
			BOOST_CHECK_EQUAL(wse1_json, wse2_json);
		}*/ 
	}

}

BOOST_AUTO_TEST_SUITE_END()
