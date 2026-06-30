#include <optional>
#include <string>
#include <string_view>

#include <boost/test/unit_test.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/write.hpp>

#include "http/webroute_metrics.h"
#include "http/server/routing/routing.h"
#include "kernel/statistics_hub.h"
#include "jandy/messages/jandy_message_ids.h"

#include "mocks/mock_beast_basicstream_with_timeout.h"
#include "support/unit_test_hublocatorinjector.h"

using namespace AqualinkAutomate;

namespace
{
	// Drive a GET request (optionally carrying a bearer token) through the routing
	// layer and materialise the resulting message_generator into a concrete
	// Response so its status/body/headers can be asserted.  Mirrors the helper in
	// test_http_security.cpp so the metrics route is exercised through the same
	// security-enforcing entry point (HTTP_OnRequest) as production traffic.
	HTTP::Response RunGet(std::string_view target, std::optional<std::string_view> bearer = std::nullopt)
	{
		HTTP::Request req;
		req.version(11);
		req.method(boost::beast::http::verb::get);
		req.target(target);
		req.set(boost::beast::http::field::host, "localhost.localdomain");
		if (bearer.has_value())
		{
			req.set(boost::beast::http::field::authorization, std::string{ "Bearer " } + std::string{ *bearer });
		}

		auto msg = HTTP::Routing::HTTP_OnRequest(req);

		boost::asio::io_context ioc;
		auto exec = ioc.get_executor();

		Test::MockBeastBasicStreamWithTimeout client_stream(exec);
		Test::MockBeastBasicStreamWithTimeout server_stream(exec);
		server_stream.connect(client_stream);

		boost::beast::error_code ec;
		boost::beast::write(server_stream, std::move(msg), ec);
		BOOST_REQUIRE_MESSAGE(!ec, "Failed to write response: " + ec.message());
		server_stream.close();

		ioc.poll();

		HTTP::Response resp;
		boost::beast::flat_buffer read_buffer;
		boost::beast::http::read(client_stream, read_buffer, resp, ec);
		BOOST_REQUIRE_MESSAGE(!ec || ec == boost::beast::http::error::end_of_stream, "Failed to read response: " + ec.message());
		return resp;
	}
}

BOOST_FIXTURE_TEST_SUITE(TestSuite_HttpRoutes_Metrics, Test::HubLocatorInjector)

// A fresh StatisticsHub yields a well-formed exposition document with the
// expected metric families present, in a stable, documented order, and served
// as Prometheus text.
BOOST_AUTO_TEST_CASE(Metrics_Exposition_FormatAndStableOrdering)
{
	HTTP::Routing::Clear();
	HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Metrics>(*this));

	auto resp = RunGet(HTTP::METRICS_ROUTE_URL);
	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());

	// Prometheus text exposition content type.
	const auto content_type = resp[boost::beast::http::field::content_type];
	BOOST_CHECK(std::string_view{ content_type }.starts_with("text/plain"));
	BOOST_CHECK(std::string_view{ content_type }.find("version=0.0.4") != std::string_view::npos);

	const std::string& body = resp.body();

	// Every family is present, each with its HELP + TYPE preamble.
	BOOST_CHECK(body.find("# TYPE aqualink_build_info gauge") != std::string::npos);
	BOOST_CHECK(body.find("# TYPE aqualink_serial_read_bytes_total counter") != std::string::npos);
	BOOST_CHECK(body.find("# TYPE aqualink_serial_write_bytes_total counter") != std::string::npos);
	BOOST_CHECK(body.find("# TYPE aqualink_messages_total counter") != std::string::npos);
	BOOST_CHECK(body.find("# TYPE aqualink_message_errors_total counter") != std::string::npos);
	BOOST_CHECK(body.find("# TYPE aqualink_serial_errors_total counter") != std::string::npos);
	BOOST_CHECK(body.find("# TYPE aqualink_latency_microseconds gauge") != std::string::npos);

	// Build info is the constant-1 info gauge with version + commit labels.
	BOOST_CHECK(body.find("aqualink_build_info{version=\"") != std::string::npos);

	// Stable ordering: family headers appear in the documented sequence.
	const auto pos_build = body.find("aqualink_build_info");
	const auto pos_read = body.find("aqualink_serial_read_bytes_total ");
	const auto pos_write = body.find("aqualink_serial_write_bytes_total ");
	const auto pos_msgs = body.find("aqualink_messages_total ");
	const auto pos_msgerr = body.find("aqualink_message_errors_total{");
	const auto pos_serr = body.find("aqualink_serial_errors_total{");
	const auto pos_lat = body.find("aqualink_latency_microseconds{");
	BOOST_CHECK_LT(pos_build, pos_read);
	BOOST_CHECK_LT(pos_read, pos_write);
	BOOST_CHECK_LT(pos_write, pos_msgs);
	BOOST_CHECK_LT(pos_msgs, pos_msgerr);
	BOOST_CHECK_LT(pos_msgerr, pos_serr);
	BOOST_CHECK_LT(pos_serr, pos_lat);

	// Latency family carries all four quantiles for each of the three stages.
	BOOST_CHECK(body.find("aqualink_latency_microseconds{stage=\"serial_read\",quantile=\"0.01\"}") != std::string::npos);
	BOOST_CHECK(body.find("aqualink_latency_microseconds{stage=\"serial_write\",quantile=\"0.5\"}") != std::string::npos);
	BOOST_CHECK(body.find("aqualink_latency_microseconds{stage=\"msg_proc\",quantile=\"0.99\"}") != std::string::npos);
}

// Counter values flow through from a populated StatisticsHub verbatim.
BOOST_AUTO_TEST_CASE(Metrics_Exposition_CounterPassthrough)
{
	auto stats = Find<Kernel::StatisticsHub>();
	BOOST_REQUIRE(nullptr != stats);

	stats->BandwidthMetrics.Read += 4096u;
	stats->BandwidthMetrics.Write += 256u;
	stats->MessageErrors.ChecksumFailures = 7u;
	stats->MessageErrors.BufferOverflows = 3u;
	stats->Serial.TransmissionFailures = 5u;
	stats->MessageCounts[Messages::JandyMessageIds::AQUARITE_Percent] += 4u;
	stats->MessageCounts[Messages::JandyMessageIds::AQUARITE_PPM] += 6u;

	HTTP::Routing::Clear();
	HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Metrics>(*this));

	auto resp = RunGet(HTTP::METRICS_ROUTE_URL);
	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());

	const std::string& body = resp.body();
	BOOST_CHECK(body.find("aqualink_serial_read_bytes_total 4096\n") != std::string::npos);
	BOOST_CHECK(body.find("aqualink_serial_write_bytes_total 256\n") != std::string::npos);
	BOOST_CHECK(body.find("aqualink_message_errors_total{kind=\"checksum\"} 7\n") != std::string::npos);
	BOOST_CHECK(body.find("aqualink_message_errors_total{kind=\"buffer_overflow\"} 3\n") != std::string::npos);
	BOOST_CHECK(body.find("aqualink_serial_errors_total{kind=\"transmission_failure\"} 5\n") != std::string::npos);
	// 4 + 6 message decodes summed across types.
	BOOST_CHECK(body.find("aqualink_messages_total 10\n") != std::string::npos);
}

// Regression: a request target carrying a query string must still route to its
// handler. The router previously parsed the whole target with parse_path, which
// rejects the '?' that begins a query, so EVERY GET with query parameters (e.g.
// /api/history/series?key=...&from=...&to=...) was answered 400 Bad Request
// before reaching its route. Routing matches on the path only; the query is
// ignored here but must not break path matching.
BOOST_AUTO_TEST_CASE(Routing_TargetWithQueryString_RoutesToHandler)
{
	HTTP::Routing::Clear();
	HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Metrics>(*this));

	auto resp = RunGet(std::string{ HTTP::METRICS_ROUTE_URL } + "?from=1&to=2&max_points=500");
	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());
	BOOST_CHECK(resp.body().find("aqualink_build_info") != std::string::npos);
}

// With a bearer token configured, /metrics is rejected without credentials and
// served with the correct token (same SecurityConfig enforcement as /api).
BOOST_AUTO_TEST_CASE(Metrics_BearerAuth_RejectsMissingTokenServesWithToken)
{
	HTTP::Routing::Clear();
	HTTP::Routing::SetSecurityConfig(HTTP::Routing::SecurityConfig{ .AuthToken = std::string{ "s3cr3t" } });
	HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Metrics>(*this));

	// No Authorization header -> 401.
	{
		auto resp = RunGet(HTTP::METRICS_ROUTE_URL);
		BOOST_CHECK_EQUAL(boost::beast::http::status::unauthorized, resp.result());
	}

	// Correct bearer token -> 200 with the exposition body.
	{
		auto resp = RunGet(HTTP::METRICS_ROUTE_URL, std::string_view{ "s3cr3t" });
		BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());
		BOOST_CHECK(resp.body().find("aqualink_build_info") != std::string::npos);
	}
}

BOOST_AUTO_TEST_SUITE_END()
