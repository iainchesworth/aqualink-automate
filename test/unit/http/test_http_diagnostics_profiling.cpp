#include <boost/test/unit_test.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>

#include "http/webroute_diagnostics_profiling.h"
#include "interfaces/iprofilingcontroller.h"
#include "kernel/hub_locator.h"

#include "mocks/mock_beast_basicstream_with_timeout.h"

using namespace AqualinkAutomate;

//=============================================================================
// Route coverage for /api/diagnostics/profiling.
//
// Drives the route's OnRequest directly against a HubLocator carrying a fake
// IProfilingController so the GET (status) and POST (start/stop/select)
// behaviour is validated without a real profiler.  Also covers the
// no-controller case (503) and the disabled-build case (409).
//=============================================================================

namespace
{
	// Minimal in-memory profiling controller for route tests.
	class FakeProfilingController : public Interfaces::IProfilingController
	{
	public:
		Status ProfilingStatus() const override { return m_Status; }

		bool Start() override
		{
			start_calls++;
			if (!m_Status.enabled) { return false; }
			m_Status.running = true;
			return true;
		}

		bool Stop() override
		{
			stop_calls++;
			if (!m_Status.enabled) { return false; }
			m_Status.running = false;
			return true;
		}

		bool SelectBackend(std::string_view backend) override
		{
			select_calls++;
			last_backend = std::string(backend);
			for (const auto& b : m_Status.available_backends)
			{
				if (b == backend)
				{
					m_Status.active_backend = std::string(backend);
					return true;
				}
			}
			return false;
		}

	public:
		// Test setup / observation.
		Status m_Status;
		int start_calls = 0;
		int stop_calls = 0;
		int select_calls = 0;
		std::string last_backend;
	};

	// Serialize a route's response and parse it back into an inspectable
	// HTTP::Response (mirrors the recording-route test technique).
	template <typename Route>
	HTTP::Response InvokeRoute(Route& route, HTTP::Request& req)
	{
		HTTP::Message msg = route.OnRequest(req);

		boost::asio::io_context ioc;
		auto exec = ioc.get_executor();

		Test::MockBeastBasicStreamWithTimeout client_stream(exec);
		Test::MockBeastBasicStreamWithTimeout server_stream(exec);
		server_stream.connect(client_stream);

		boost::beast::error_code ec;
		boost::beast::write(server_stream, std::move(msg), ec);
		if (ec)
		{
			throw std::runtime_error("Failed to write response: " + ec.message());
		}
		server_stream.close();
		ioc.poll();

		HTTP::Response resp;
		boost::beast::flat_buffer read_buffer;
		boost::beast::http::read(client_stream, read_buffer, resp, ec);
		if (ec && ec != boost::beast::http::error::end_of_stream)
		{
			throw std::runtime_error("Failed to read response: " + ec.message());
		}
		return resp;
	}

	HTTP::Request MakeGet()
	{
		HTTP::Request req;
		req.version(11);
		req.method(boost::beast::http::verb::get);
		req.target("/api/diagnostics/profiling");
		req.set(boost::beast::http::field::host, "localhost.localdomain");
		return req;
	}

	HTTP::Request MakePost(const std::string& body)
	{
		HTTP::Request req;
		req.version(11);
		req.method(boost::beast::http::verb::post);
		req.target("/api/diagnostics/profiling");
		req.set(boost::beast::http::field::host, "localhost.localdomain");
		req.set(boost::beast::http::field::content_type, "application/json");
		req.body() = body;
		req.prepare_payload();
		return req;
	}

	std::shared_ptr<FakeProfilingController> MakeEnabledController()
	{
		auto controller = std::make_shared<FakeProfilingController>();
		controller->m_Status.enabled = true;
		controller->m_Status.running = true;
		controller->m_Status.active_backend = "tracy";
		controller->m_Status.available_backends = { "tracy", "vtune" };
		return controller;
	}
}

BOOST_AUTO_TEST_SUITE(TestSuite_HttpDiagnosticsProfiling)

//-----------------------------------------------------------------------------
// GET reports {enabled,running,backend,available}.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Test_Profiling_Get_ReportsStatus)
{
	Kernel::HubLocator hub_locator;
	hub_locator.Register<Interfaces::IProfilingController>(MakeEnabledController());

	HTTP::WebRoute_Diagnostics_Profiling route(hub_locator);

	auto req = MakeGet();
	auto resp = InvokeRoute(route, req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());

	auto json = nlohmann::json::parse(resp.body());
	BOOST_REQUIRE(json.contains("enabled"));
	BOOST_REQUIRE(json.contains("running"));
	BOOST_REQUIRE(json.contains("backend"));
	BOOST_REQUIRE(json.contains("available"));
	BOOST_CHECK_EQUAL(json["enabled"].get<bool>(), true);
	BOOST_CHECK_EQUAL(json["backend"].get<std::string>(), "tracy");
	BOOST_CHECK_EQUAL(json["available"].size(), 2u);
}

//-----------------------------------------------------------------------------
// POST stop then start toggles running and reflects it in the response.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Test_Profiling_PostStopStart_TogglesRunning)
{
	Kernel::HubLocator hub_locator;
	auto controller = MakeEnabledController();
	hub_locator.Register<Interfaces::IProfilingController>(controller);

	HTTP::WebRoute_Diagnostics_Profiling route(hub_locator);

	{
		auto req = MakePost(R"({"action":"stop"})");
		auto resp = InvokeRoute(route, req);
		BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());
		BOOST_CHECK_EQUAL(controller->stop_calls, 1);
		BOOST_CHECK_EQUAL(nlohmann::json::parse(resp.body())["running"].get<bool>(), false);
	}
	{
		auto req = MakePost(R"({"action":"start"})");
		auto resp = InvokeRoute(route, req);
		BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());
		BOOST_CHECK_EQUAL(controller->start_calls, 1);
		BOOST_CHECK_EQUAL(nlohmann::json::parse(resp.body())["running"].get<bool>(), true);
	}
}

//-----------------------------------------------------------------------------
// POST select switches the active backend; an unavailable backend is 409.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Test_Profiling_PostSelect_Backend)
{
	Kernel::HubLocator hub_locator;
	auto controller = MakeEnabledController();
	hub_locator.Register<Interfaces::IProfilingController>(controller);

	HTTP::WebRoute_Diagnostics_Profiling route(hub_locator);

	{
		auto req = MakePost(R"({"action":"select","backend":"vtune"})");
		auto resp = InvokeRoute(route, req);
		BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());
		BOOST_CHECK_EQUAL(controller->last_backend, "vtune");
		BOOST_CHECK_EQUAL(nlohmann::json::parse(resp.body())["backend"].get<std::string>(), "vtune");
	}
	{
		// uprof is not in available_backends -> controller refuses -> 409.
		auto req = MakePost(R"({"action":"select","backend":"uprof"})");
		auto resp = InvokeRoute(route, req);
		BOOST_CHECK_EQUAL(boost::beast::http::status::conflict, resp.result());
	}
	{
		// select without backend -> 400.
		auto req = MakePost(R"({"action":"select"})");
		auto resp = InvokeRoute(route, req);
		BOOST_CHECK_EQUAL(boost::beast::http::status::bad_request, resp.result());
	}
}

//-----------------------------------------------------------------------------
// When no backend is compiled in, start/stop are rejected with 409.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Test_Profiling_Disabled_StartStop_Conflict)
{
	Kernel::HubLocator hub_locator;
	auto controller = std::make_shared<FakeProfilingController>();  // enabled=false by default
	hub_locator.Register<Interfaces::IProfilingController>(controller);

	HTTP::WebRoute_Diagnostics_Profiling route(hub_locator);

	auto req = MakePost(R"({"action":"start"})");
	auto resp = InvokeRoute(route, req);
	BOOST_CHECK_EQUAL(boost::beast::http::status::conflict, resp.result());
}

//-----------------------------------------------------------------------------
// Malformed / invalid requests are 400.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Test_Profiling_Post_BadRequests)
{
	Kernel::HubLocator hub_locator;
	hub_locator.Register<Interfaces::IProfilingController>(MakeEnabledController());

	HTTP::WebRoute_Diagnostics_Profiling route(hub_locator);

	for (const auto* body : { "not-json", R"({"foo":1})", R"({"action":"frobnicate"})" })
	{
		auto req = MakePost(body);
		auto resp = InvokeRoute(route, req);
		BOOST_CHECK_MESSAGE(boost::beast::http::status::bad_request == resp.result(),
			"Expected 400 for body: " << body);
	}
}

//-----------------------------------------------------------------------------
// Non GET/POST verbs are 405.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Test_Profiling_MethodNotAllowed)
{
	Kernel::HubLocator hub_locator;
	hub_locator.Register<Interfaces::IProfilingController>(MakeEnabledController());

	HTTP::WebRoute_Diagnostics_Profiling route(hub_locator);

	HTTP::Request req;
	req.version(11);
	req.method(boost::beast::http::verb::delete_);
	req.target("/api/diagnostics/profiling");
	req.set(boost::beast::http::field::host, "localhost.localdomain");

	auto resp = InvokeRoute(route, req);
	BOOST_CHECK_EQUAL(boost::beast::http::status::method_not_allowed, resp.result());
}

//-----------------------------------------------------------------------------
// No controller registered: GET reports enabled=false and POST returns 503.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Test_Profiling_NoController_GetFalse_PostUnavailable)
{
	Kernel::HubLocator hub_locator; // no IProfilingController registered

	HTTP::WebRoute_Diagnostics_Profiling route(hub_locator);

	auto get_req = MakeGet();
	auto get_resp = InvokeRoute(route, get_req);
	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, get_resp.result());
	BOOST_CHECK_EQUAL(nlohmann::json::parse(get_resp.body())["enabled"].get<bool>(), false);

	auto post_req = MakePost(R"({"action":"start"})");
	auto post_resp = InvokeRoute(route, post_req);
	BOOST_CHECK_EQUAL(boost::beast::http::status::service_unavailable, post_resp.result());
}

BOOST_AUTO_TEST_SUITE_END()
