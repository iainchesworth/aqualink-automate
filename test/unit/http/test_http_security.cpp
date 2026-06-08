#include <string>
#include <string_view>
#include <utility>

#include <boost/test/unit_test.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/write.hpp>

#include "http/server/routing/routing.h"
#include "http/server/server_types.h"
#include "interfaces/iwebroute.h"

#include "mocks/mock_beast_basicstream_with_timeout.h"

using namespace AqualinkAutomate;

// Minimal HTTP route handler that always answers 200 OK. Used to confirm that a
// request which passes (or bypasses) the security policy is dispatched normally.
// (File-scope inline constexpr matches the route-URL pattern used by the real
// webroutes so it is usable as a non-type template argument.)
inline constexpr char OK_ROUTE_URL[] = "/secure/ping";

namespace
{

	class TestOkRoute : public Interfaces::IWebRoute<OK_ROUTE_URL>
	{
	public:
		HTTP::Message OnRequest(const HTTP::Request& req) override
		{
			HTTP::Response res{ boost::beast::http::status::ok, req.version() };
			res.keep_alive(req.keep_alive());
			res.body() = "pong";
			res.prepare_payload();
			return res;
		}
	};

	// Drive a fully-formed request through the routing layer and materialise the
	// resulting message_generator into a concrete Response so its status can be
	// asserted. Mirrors Test::PerformHttpRequestResponse but lets the caller set
	// arbitrary headers/method (needed to exercise the security policy).
	HTTP::Response RunRequest(HTTP::Request& req)
	{
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

	HTTP::Request MakeRequest(boost::beast::http::verb method, std::string_view target)
	{
		HTTP::Request req;
		req.version(11);
		req.method(method);
		req.target(target);
		req.set(boost::beast::http::field::host, "localhost.localdomain");
		return req;
	}

	void RegisterOkRoute()
	{
		HTTP::Routing::Clear();
		HTTP::Routing::Add(std::make_unique<TestOkRoute>());
	}

}
// unnamed namespace

BOOST_AUTO_TEST_SUITE(TestSuite_HttpSecurity)

// =============================================================================
// Backward compatibility: no policy installed => identical to historical behaviour
// =============================================================================

BOOST_AUTO_TEST_CASE(Test_Security_Disabled_RequestPassesThrough)
{
	RegisterOkRoute();
	// Default config (Clear() reset it) -> no auth, no origin check, no CSRF.
	BOOST_CHECK(!HTTP::Routing::GetSecurityConfig().IsEnabled());

	auto req = MakeRequest(boost::beast::http::verb::get, OK_ROUTE_URL);
	auto resp = RunRequest(req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());
	BOOST_CHECK_EQUAL("pong", resp.body());

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Security_Clear_ResetsPolicy)
{
	HTTP::Routing::Clear();

	HTTP::Routing::SecurityConfig cfg;
	cfg.AuthToken = "x";
	HTTP::Routing::SetSecurityConfig(cfg);
	BOOST_CHECK(HTTP::Routing::GetSecurityConfig().IsEnabled());

	// Clear() must reset the policy to the disabled default for test isolation.
	HTTP::Routing::Clear();
	BOOST_CHECK(!HTTP::Routing::GetSecurityConfig().IsEnabled());
}

// =============================================================================
// Bearer-token authentication
// =============================================================================

BOOST_AUTO_TEST_CASE(Test_Security_Auth_MissingToken_Returns401)
{
	RegisterOkRoute();

	HTTP::Routing::SecurityConfig cfg;
	cfg.AuthToken = "s3cr3t-token";
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::get, OK_ROUTE_URL);
	auto resp = RunRequest(req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::unauthorized, resp.result());

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Security_Auth_WrongToken_Returns401)
{
	RegisterOkRoute();

	HTTP::Routing::SecurityConfig cfg;
	cfg.AuthToken = "s3cr3t-token";
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::get, OK_ROUTE_URL);
	req.set(boost::beast::http::field::authorization, "Bearer not-the-token");
	auto resp = RunRequest(req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::unauthorized, resp.result());

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Security_Auth_WrongScheme_Returns401)
{
	RegisterOkRoute();

	HTTP::Routing::SecurityConfig cfg;
	cfg.AuthToken = "s3cr3t-token";
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::get, OK_ROUTE_URL);
	// Correct token value but wrong scheme (Basic instead of Bearer).
	req.set(boost::beast::http::field::authorization, "Basic s3cr3t-token");
	auto resp = RunRequest(req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::unauthorized, resp.result());

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Security_Auth_CorrectToken_PassesThrough)
{
	RegisterOkRoute();

	HTTP::Routing::SecurityConfig cfg;
	cfg.AuthToken = "s3cr3t-token";
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::get, OK_ROUTE_URL);
	req.set(boost::beast::http::field::authorization, "Bearer s3cr3t-token");
	auto resp = RunRequest(req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());
	BOOST_CHECK_EQUAL("pong", resp.body());

	HTTP::Routing::Clear();
}

// =============================================================================
// Origin allow-list
// =============================================================================

BOOST_AUTO_TEST_CASE(Test_Security_Origin_Disallowed_Returns403)
{
	RegisterOkRoute();

	HTTP::Routing::SecurityConfig cfg;
	cfg.AllowedOrigins = { "https://trusted.example" };
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::get, OK_ROUTE_URL);
	req.set(boost::beast::http::field::origin, "https://evil.example");
	auto resp = RunRequest(req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::forbidden, resp.result());

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Security_Origin_Missing_Returns403)
{
	RegisterOkRoute();

	HTTP::Routing::SecurityConfig cfg;
	cfg.AllowedOrigins = { "https://trusted.example" };
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::get, OK_ROUTE_URL);
	auto resp = RunRequest(req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::forbidden, resp.result());

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Security_Origin_Allowed_PassesThrough)
{
	RegisterOkRoute();

	HTTP::Routing::SecurityConfig cfg;
	cfg.AllowedOrigins = { "https://trusted.example" };
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::get, OK_ROUTE_URL);
	req.set(boost::beast::http::field::origin, "https://trusted.example");
	auto resp = RunRequest(req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());

	HTTP::Routing::Clear();
}

// =============================================================================
// CSRF custom-header requirement (state-changing methods only)
// =============================================================================

BOOST_AUTO_TEST_CASE(Test_Security_Csrf_PostWithoutHeader_Returns403)
{
	RegisterOkRoute();

	HTTP::Routing::SecurityConfig cfg;
	cfg.RequireCsrfHeader = true;
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::post, OK_ROUTE_URL);
	auto resp = RunRequest(req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::forbidden, resp.result());

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Security_Csrf_PostWithHeader_PassesThrough)
{
	RegisterOkRoute();

	HTTP::Routing::SecurityConfig cfg;
	cfg.RequireCsrfHeader = true;
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::post, OK_ROUTE_URL);
	req.set("X-Requested-With", "XMLHttpRequest");
	auto resp = RunRequest(req);

	// The route does not distinguish methods, so a passing POST still gets 200.
	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Security_Csrf_GetUnaffected)
{
	RegisterOkRoute();

	HTTP::Routing::SecurityConfig cfg;
	cfg.RequireCsrfHeader = true;
	HTTP::Routing::SetSecurityConfig(cfg);

	// GET is not state-changing; the CSRF requirement must not apply.
	auto req = MakeRequest(boost::beast::http::verb::get, OK_ROUTE_URL);
	auto resp = RunRequest(req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());

	HTTP::Routing::Clear();
}

// =============================================================================
// WebSocket-upgrade gating (AuthorizeWebSocketUpgrade)
// =============================================================================

BOOST_AUTO_TEST_CASE(Test_Security_WsUpgrade_DisabledByDefault_Allowed)
{
	HTTP::Routing::Clear();   // default (disabled) policy

	auto req = MakeRequest(boost::beast::http::verb::get, "/ws/equipment");
	auto rejection = HTTP::Routing::AuthorizeWebSocketUpgrade(req);
	BOOST_CHECK(!rejection.has_value());

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Security_WsUpgrade_MissingToken_Returns401)
{
	HTTP::Routing::Clear();

	HTTP::Routing::SecurityConfig cfg;
	cfg.AuthToken = "ws-token";
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::get, "/ws/equipment");
	auto rejection = HTTP::Routing::AuthorizeWebSocketUpgrade(req);
	BOOST_REQUIRE(rejection.has_value());
	BOOST_CHECK_EQUAL(boost::beast::http::status::unauthorized, rejection->result());

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Security_WsUpgrade_CorrectToken_Allowed)
{
	HTTP::Routing::Clear();

	HTTP::Routing::SecurityConfig cfg;
	cfg.AuthToken = "ws-token";
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::get, "/ws/equipment");
	req.set(boost::beast::http::field::authorization, "Bearer ws-token");
	auto rejection = HTTP::Routing::AuthorizeWebSocketUpgrade(req);
	BOOST_CHECK(!rejection.has_value());

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Security_WsUpgrade_BadOrigin_Returns403)
{
	HTTP::Routing::Clear();

	HTTP::Routing::SecurityConfig cfg;
	cfg.AllowedOrigins = { "https://trusted.example" };
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::get, "/ws/equipment");
	req.set(boost::beast::http::field::origin, "https://evil.example");
	auto rejection = HTTP::Routing::AuthorizeWebSocketUpgrade(req);
	BOOST_REQUIRE(rejection.has_value());
	BOOST_CHECK_EQUAL(boost::beast::http::status::forbidden, rejection->result());

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_SUITE_END()
