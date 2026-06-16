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

#include <filesystem>
#include <fstream>
#include <system_error>

#include "http/server/routing/routing.h"
#include "http/server/server_types.h"
#include "http/server/static_file_handler.h"
#include "http/webroute_auth_check.h"
#include "http/webroute_health.h"
#include "http/webroute_health_detailed.h"
#include "interfaces/iwebroute.h"

#include "mocks/mock_beast_basicstream_with_timeout.h"
#include "support/unit_test_hublocatorinjector.h"

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
		HTTP::Response OnRequest(const HTTP::Request& req) override
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

// =============================================================================
// WebSocket subprotocol bearer auth (browsers cannot set Authorization on a WS
// upgrade, so the token rides in Sec-WebSocket-Protocol as "bearer.<token>").
// =============================================================================

BOOST_AUTO_TEST_CASE(Test_Security_WsUpgrade_SubprotocolToken_Allowed)
{
	HTTP::Routing::Clear();
	HTTP::Routing::SecurityConfig cfg;
	cfg.AuthToken = "ws-token";
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::get, "/ws/equipment");
	req.set(boost::beast::http::field::sec_websocket_protocol, "aqualink, bearer.ws-token");
	BOOST_CHECK(!HTTP::Routing::AuthorizeWebSocketUpgrade(req).has_value());

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Security_WsUpgrade_SubprotocolToken_NoSpaceVariant_Allowed)
{
	HTTP::Routing::Clear();
	HTTP::Routing::SecurityConfig cfg;
	cfg.AuthToken = "ws-token";
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::get, "/ws/equipment");
	req.set(boost::beast::http::field::sec_websocket_protocol, "aqualink,bearer.ws-token");
	BOOST_CHECK(!HTTP::Routing::AuthorizeWebSocketUpgrade(req).has_value());

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Security_WsUpgrade_WrongSubprotocolToken_Returns401)
{
	HTTP::Routing::Clear();
	HTTP::Routing::SecurityConfig cfg;
	cfg.AuthToken = "ws-token";
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::get, "/ws/equipment");
	req.set(boost::beast::http::field::sec_websocket_protocol, "aqualink, bearer.wrong-token");
	auto rejection = HTTP::Routing::AuthorizeWebSocketUpgrade(req);
	BOOST_REQUIRE(rejection.has_value());
	BOOST_CHECK_EQUAL(boost::beast::http::status::unauthorized, rejection->result());

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_Security_WsUpgrade_SubprotocolNoBearerEntry_Returns401)
{
	HTTP::Routing::Clear();
	HTTP::Routing::SecurityConfig cfg;
	cfg.AuthToken = "ws-token";
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::get, "/ws/equipment");
	req.set(boost::beast::http::field::sec_websocket_protocol, "aqualink");
	auto rejection = HTTP::Routing::AuthorizeWebSocketUpgrade(req);
	BOOST_REQUIRE(rejection.has_value());
	BOOST_CHECK_EQUAL(boost::beast::http::status::unauthorized, rejection->result());

	HTTP::Routing::Clear();
}

// The subprotocol token path is WS-upgrade-only: a plain HTTP request carrying a
// bearer.<token> subprotocol but no Authorization header must still be rejected.
BOOST_AUTO_TEST_CASE(Test_Security_Http_SubprotocolTokenIgnored_Returns401)
{
	RegisterOkRoute();
	HTTP::Routing::SecurityConfig cfg;
	cfg.AuthToken = "ws-token";
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::get, OK_ROUTE_URL);
	req.set(boost::beast::http::field::sec_websocket_protocol, "aqualink, bearer.ws-token");
	auto resp = RunRequest(req);
	BOOST_CHECK_EQUAL(boost::beast::http::status::unauthorized, resp.result());

	HTTP::Routing::Clear();
}

// =============================================================================
// Static assets are served WITHOUT authentication (so the login screen loads).
// =============================================================================

BOOST_AUTO_TEST_CASE(Test_Security_StaticAsset_ServedWithoutAuth)
{
	HTTP::Routing::Clear();

	const auto doc_root = std::filesystem::temp_directory_path() / "aqualink_ws5_static_test";
	std::filesystem::create_directories(doc_root);
	{
		std::ofstream out((doc_root / "index.html").string(), std::ios::binary);
		out << "<html>login shell</html>";
	}

	HTTP::Routing::SecurityConfig cfg;
	cfg.AuthToken = "the-token";
	HTTP::Routing::SetSecurityConfig(cfg);
	HTTP::Routing::StaticHandler(HTTP::StaticFileHandler("/", doc_root.string()));

	// No Authorization header, token required -> static file must STILL be served.
	auto req = MakeRequest(boost::beast::http::verb::get, "/index.html");
	auto resp = RunRequest(req);
	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());

	HTTP::Routing::Clear();
	std::error_code ec;
	std::filesystem::remove_all(doc_root, ec);
}

// =============================================================================
// /api/auth/check probe.
// =============================================================================

BOOST_AUTO_TEST_CASE(Test_AuthCheck_AuthDisabled_Returns200)
{
	HTTP::Routing::Clear();
	HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_AuthCheck>());

	auto req = MakeRequest(boost::beast::http::verb::get, HTTP::AUTH_CHECK_ROUTE_URL);
	auto resp = RunRequest(req);
	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());
	BOOST_CHECK(resp.body().find("authenticated") != std::string::npos);

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_AuthCheck_TokenSetNoCredentials_Returns401)
{
	HTTP::Routing::Clear();
	HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_AuthCheck>());
	HTTP::Routing::SecurityConfig cfg;
	cfg.AuthToken = "the-token";
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::get, HTTP::AUTH_CHECK_ROUTE_URL);
	auto resp = RunRequest(req);
	BOOST_CHECK_EQUAL(boost::beast::http::status::unauthorized, resp.result());

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_CASE(Test_AuthCheck_TokenSetCorrectBearer_Returns200)
{
	HTTP::Routing::Clear();
	HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_AuthCheck>());
	HTTP::Routing::SecurityConfig cfg;
	cfg.AuthToken = "the-token";
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::get, HTTP::AUTH_CHECK_ROUTE_URL);
	req.set(boost::beast::http::field::authorization, "Bearer the-token");
	auto resp = RunRequest(req);
	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());

	HTTP::Routing::Clear();
}

// =============================================================================
// /api/health liveness probe. Unlike every other route it is auth-EXEMPT
// (RequiresAuthentication() -> false) so a container/orchestrator health check
// reaches it without the operator's bearer token.
// =============================================================================

BOOST_AUTO_TEST_CASE(Test_Health_AuthDisabled_Returns200WithStatusOk)
{
	HTTP::Routing::Clear();
	HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Health>());

	auto req = MakeRequest(boost::beast::http::verb::get, HTTP::HEALTH_ROUTE_URL);
	auto resp = RunRequest(req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());
	BOOST_CHECK(resp.body().find("\"status\":\"ok\"") != std::string::npos);
	BOOST_CHECK(resp.body().find("uptime_seconds") != std::string::npos);

	HTTP::Routing::Clear();
}

// The Docker-healthcheck guarantee: a token-protected server must STILL serve
// /api/health to a request carrying NO credentials (other routes would 401 here).
BOOST_AUTO_TEST_CASE(Test_Health_TokenSetNoCredentials_StillReturns200)
{
	HTTP::Routing::Clear();
	HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Health>());
	HTTP::Routing::SecurityConfig cfg;
	cfg.AuthToken = "the-token";
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::get, HTTP::HEALTH_ROUTE_URL);
	auto resp = RunRequest(req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());

	HTTP::Routing::Clear();
}

// The exemption bypasses the WHOLE policy, not just the bearer check: with an
// Origin allow-list configured and no Origin header (which 403s a gated route),
// the probe must still pass.
BOOST_AUTO_TEST_CASE(Test_Health_OriginAllowlist_StillReturns200)
{
	HTTP::Routing::Clear();
	HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Health>());
	HTTP::Routing::SecurityConfig cfg;
	cfg.AllowedOrigins = { "https://trusted.example" };
	HTTP::Routing::SetSecurityConfig(cfg);

	auto req = MakeRequest(boost::beast::http::verb::get, HTTP::HEALTH_ROUTE_URL);
	auto resp = RunRequest(req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());

	HTTP::Routing::Clear();
}

// With BOTH health routes registered and a token required, the router must resolve
// the two prefix-sharing paths to their distinct handlers AND apply per-route auth:
// the exempt liveness probe answers 200 unauthenticated, while the gated detailed
// route answers 401. This guards against a trie collision serving the detailed
// (internal-state) report without authentication.
BOOST_AUTO_TEST_CASE(Test_Health_BothRoutes_PerRouteAuthEnforced)
{
	Test::HubLocatorInjector hub_locator;

	HTTP::Routing::Clear();
	HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_Health>());
	HTTP::Routing::Add(std::make_unique<HTTP::WebRoute_HealthDetailed>(hub_locator));
	HTTP::Routing::SecurityConfig cfg;
	cfg.AuthToken = "the-token";
	HTTP::Routing::SetSecurityConfig(cfg);

	// Liveness probe: exempt -> 200 without credentials.
	auto health_req = MakeRequest(boost::beast::http::verb::get, HTTP::HEALTH_ROUTE_URL);
	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, RunRequest(health_req).result());

	// Detailed report: gated -> 401 without credentials.
	auto detailed_req = MakeRequest(boost::beast::http::verb::get, HTTP::HEALTH_DETAILED_ROUTE_URL);
	BOOST_CHECK_EQUAL(boost::beast::http::status::unauthorized, RunRequest(detailed_req).result());

	// With the correct token the detailed route is reachable (503 here: the injector's
	// DataHub has no pool configuration, so the system reports not-ready).
	detailed_req.set(boost::beast::http::field::authorization, "Bearer the-token");
	BOOST_CHECK_EQUAL(boost::beast::http::status::service_unavailable, RunRequest(detailed_req).result());

	HTTP::Routing::Clear();
}

BOOST_AUTO_TEST_SUITE_END()
