#include <boost/test/unit_test.hpp>

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
	/// Verify that two concurrent WebSocket upgrades to /ws/equipment succeed.

	{
		auto [websocket1_buffer, bytes_read1] = Test::PerformHttpWsUpgradeResponse("/ws/equipment", [&hub_locator]() -> void
			{
				// Upgrade-only test; no data exchange.
			});
		auto [websocket2_buffer, bytes_read2] = Test::PerformHttpWsUpgradeResponse("/ws/equipment", [&hub_locator]() -> void
			{
				// Upgrade-only test; no data exchange.
			});
	}

}

BOOST_AUTO_TEST_SUITE_END()
