#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <memory>
#include <string>

#include <boost/asio/io_context.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>

#include "http/webroute_diagnostics_recording.h"
#include "interfaces/irecordingcontroller.h"
#include "kernel/hub_locator.h"

#include "mocks/mock_beast_basicstream_with_timeout.h"

using namespace AqualinkAutomate;

//=============================================================================
// Route coverage for /api/diagnostics/recording.
//
// Drives the route's OnRequest directly against a HubLocator carrying a fake
// IRecordingController so the GET (status) and POST (start/stop) behaviour is
// validated without a real serial port.  Also covers the no-controller case
// (dev-mode/replay) where toggling must be rejected with 503.
//=============================================================================

namespace
{
	// Minimal in-memory recording controller for route tests.
	class FakeRecordingController : public Interfaces::IRecordingController
	{
	public:
		bool StartRecording(const std::string& filename) override
		{
			start_calls++;
			last_start_filename = filename;
			if (fail_start) { return false; }
			if (m_Recording) { return false; }
			m_Recording = true;
			m_Status.recording = true;
			m_Status.file = filename;
			m_Status.bytes_written = 0;
			return true;
		}

		bool StopRecording() override
		{
			stop_calls++;
			if (!m_Recording) { return false; }
			m_Recording = false;
			m_Status.recording = false;
			// File path + byte count retained so a post-stop GET still reports them.
			return true;
		}

		bool IsRecording() const override { return m_Recording; }

		Status RecordingStatus() const override { return m_Status; }

	public:
		// Test knobs / observation.
		void SetBytes(std::size_t n) { m_Status.bytes_written = n; }

		bool fail_start = false;
		int start_calls = 0;
		int stop_calls = 0;
		std::string last_start_filename;

	private:
		bool m_Recording = false;
		Status m_Status;
	};

	// Serialize a route's message_generator response and parse it back into an
	// inspectable HTTP::Response (status code + body).  Mirrors the technique in
	// PerformHttpRequestResponse but works for an arbitrary request/route, which
	// the GET-only shared helper does not support.
	HTTP::Response InvokeRoute(HTTP::WebRoute_Diagnostics_Recording& route, HTTP::Request& req)
	{
		auto msg = route.OnRequest(req);

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
		req.target("/api/diagnostics/recording");
		req.set(boost::beast::http::field::host, "localhost.localdomain");
		return req;
	}

	HTTP::Request MakePost(const std::string& body)
	{
		HTTP::Request req;
		req.version(11);
		req.method(boost::beast::http::verb::post);
		req.target("/api/diagnostics/recording");
		req.set(boost::beast::http::field::host, "localhost.localdomain");
		req.set(boost::beast::http::field::content_type, "application/json");
		req.body() = body;
		req.prepare_payload();
		return req;
	}
}

BOOST_AUTO_TEST_SUITE(TestSuite_HttpDiagnosticsRecording)

//-----------------------------------------------------------------------------
// GET reports the controller's current status as {recording,file,bytes}.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Test_Recording_Get_ReportsStatus)
{
	Kernel::HubLocator hub_locator;
	auto controller = std::make_shared<FakeRecordingController>();
	hub_locator.Register<Interfaces::IRecordingController>(controller);

	HTTP::WebRoute_Diagnostics_Recording route(hub_locator);

	auto req = MakeGet();
	auto resp = InvokeRoute(route, req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());

	auto json = nlohmann::json::parse(resp.body());
	BOOST_REQUIRE(json.contains("recording"));
	BOOST_REQUIRE(json.contains("file"));
	BOOST_REQUIRE(json.contains("bytes"));
	BOOST_CHECK_EQUAL(json["recording"].get<bool>(), false);
	BOOST_CHECK_EQUAL(json["file"].get<std::string>(), "");
	BOOST_CHECK_EQUAL(json["bytes"].get<std::size_t>(), 0u);
}

//-----------------------------------------------------------------------------
// POST start begins recording and the response reflects the new status; a
// follow-up GET reports the live byte count.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Test_Recording_PostStart_StartsAndReportsStatus)
{
	Kernel::HubLocator hub_locator;
	auto controller = std::make_shared<FakeRecordingController>();
	hub_locator.Register<Interfaces::IRecordingController>(controller);

	HTTP::WebRoute_Diagnostics_Recording route(hub_locator);

	auto req = MakePost(R"({"action":"start","filename":"session.cap"})");
	auto resp = InvokeRoute(route, req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());
	BOOST_CHECK_EQUAL(controller->start_calls, 1);
	BOOST_CHECK_EQUAL(controller->last_start_filename, "session.cap");
	BOOST_CHECK(controller->IsRecording());

	auto json = nlohmann::json::parse(resp.body());
	BOOST_CHECK_EQUAL(json["recording"].get<bool>(), true);
	BOOST_CHECK_EQUAL(json["file"].get<std::string>(), "session.cap");

	// Simulate bytes accruing on the wire, then GET reflects them.
	controller->SetBytes(128);
	auto get_req = MakeGet();
	auto get_resp = InvokeRoute(route, get_req);
	auto get_json = nlohmann::json::parse(get_resp.body());
	BOOST_CHECK_EQUAL(get_json["bytes"].get<std::size_t>(), 128u);
}

//-----------------------------------------------------------------------------
// POST stop ends recording.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Test_Recording_PostStop_Stops)
{
	Kernel::HubLocator hub_locator;
	auto controller = std::make_shared<FakeRecordingController>();
	hub_locator.Register<Interfaces::IRecordingController>(controller);

	HTTP::WebRoute_Diagnostics_Recording route(hub_locator);

	// Start, then stop.
	{
		auto req = MakePost(R"({"action":"start","filename":"s.cap"})");
		auto resp = InvokeRoute(route, req);
		BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());
	}

	auto req = MakePost(R"({"action":"stop"})");
	auto resp = InvokeRoute(route, req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());
	BOOST_CHECK_EQUAL(controller->stop_calls, 1);
	BOOST_CHECK(!controller->IsRecording());

	auto json = nlohmann::json::parse(resp.body());
	BOOST_CHECK_EQUAL(json["recording"].get<bool>(), false);
}

//-----------------------------------------------------------------------------
// Starting while already recording is a 409 conflict.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Test_Recording_PostStart_WhenAlreadyRecording_Conflict)
{
	Kernel::HubLocator hub_locator;
	auto controller = std::make_shared<FakeRecordingController>();
	hub_locator.Register<Interfaces::IRecordingController>(controller);

	HTTP::WebRoute_Diagnostics_Recording route(hub_locator);

	{
		auto req = MakePost(R"({"action":"start","filename":"a.cap"})");
		auto resp = InvokeRoute(route, req);
		BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());
	}

	auto req = MakePost(R"({"action":"start","filename":"b.cap"})");
	auto resp = InvokeRoute(route, req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::conflict, resp.result());
	auto json = nlohmann::json::parse(resp.body());
	BOOST_REQUIRE(json.contains("error"));
}

//-----------------------------------------------------------------------------
// Stopping when nothing is recording is a 409 conflict.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Test_Recording_PostStop_WhenNotRecording_Conflict)
{
	Kernel::HubLocator hub_locator;
	auto controller = std::make_shared<FakeRecordingController>();
	hub_locator.Register<Interfaces::IRecordingController>(controller);

	HTTP::WebRoute_Diagnostics_Recording route(hub_locator);

	auto req = MakePost(R"({"action":"stop"})");
	auto resp = InvokeRoute(route, req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::conflict, resp.result());
}

//-----------------------------------------------------------------------------
// Malformed / invalid requests are 400.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Test_Recording_Post_BadRequests)
{
	Kernel::HubLocator hub_locator;
	auto controller = std::make_shared<FakeRecordingController>();
	hub_locator.Register<Interfaces::IRecordingController>(controller);

	HTTP::WebRoute_Diagnostics_Recording route(hub_locator);

	// Invalid JSON.
	{
		auto req = MakePost("not-json");
		auto resp = InvokeRoute(route, req);
		BOOST_CHECK_EQUAL(boost::beast::http::status::bad_request, resp.result());
	}
	// Missing action.
	{
		auto req = MakePost(R"({"filename":"x.cap"})");
		auto resp = InvokeRoute(route, req);
		BOOST_CHECK_EQUAL(boost::beast::http::status::bad_request, resp.result());
	}
	// Unknown action.
	{
		auto req = MakePost(R"({"action":"pause"})");
		auto resp = InvokeRoute(route, req);
		BOOST_CHECK_EQUAL(boost::beast::http::status::bad_request, resp.result());
	}
	// start without filename.
	{
		auto req = MakePost(R"({"action":"start"})");
		auto resp = InvokeRoute(route, req);
		BOOST_CHECK_EQUAL(boost::beast::http::status::bad_request, resp.result());
	}
	// start with empty filename.
	{
		auto req = MakePost(R"({"action":"start","filename":""})");
		auto resp = InvokeRoute(route, req);
		BOOST_CHECK_EQUAL(boost::beast::http::status::bad_request, resp.result());
	}

	BOOST_CHECK_EQUAL(controller->start_calls, 0);
}

//-----------------------------------------------------------------------------
// A start that the controller refuses (e.g. file could not be opened) is 409.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Test_Recording_PostStart_ControllerRefuses_Conflict)
{
	Kernel::HubLocator hub_locator;
	auto controller = std::make_shared<FakeRecordingController>();
	controller->fail_start = true;
	hub_locator.Register<Interfaces::IRecordingController>(controller);

	HTTP::WebRoute_Diagnostics_Recording route(hub_locator);

	auto req = MakePost(R"({"action":"start","filename":"/no/such/dir/x.cap"})");
	auto resp = InvokeRoute(route, req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::conflict, resp.result());
	BOOST_CHECK_EQUAL(controller->start_calls, 1);
}

//-----------------------------------------------------------------------------
// Non GET/POST verbs are 405.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Test_Recording_MethodNotAllowed)
{
	Kernel::HubLocator hub_locator;
	auto controller = std::make_shared<FakeRecordingController>();
	hub_locator.Register<Interfaces::IRecordingController>(controller);

	HTTP::WebRoute_Diagnostics_Recording route(hub_locator);

	HTTP::Request req;
	req.version(11);
	req.method(boost::beast::http::verb::delete_);
	req.target("/api/diagnostics/recording");
	req.set(boost::beast::http::field::host, "localhost.localdomain");

	auto resp = InvokeRoute(route, req);
	BOOST_CHECK_EQUAL(boost::beast::http::status::method_not_allowed, resp.result());
}

//-----------------------------------------------------------------------------
// No controller registered (dev-mode/replay): GET reports recording=false and
// POST toggles are rejected with 503.
//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(Test_Recording_NoController_GetFalse_PostUnavailable)
{
	Kernel::HubLocator hub_locator; // no IRecordingController registered

	HTTP::WebRoute_Diagnostics_Recording route(hub_locator);

	auto get_req = MakeGet();
	auto get_resp = InvokeRoute(route, get_req);
	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, get_resp.result());
	auto get_json = nlohmann::json::parse(get_resp.body());
	BOOST_CHECK_EQUAL(get_json["recording"].get<bool>(), false);

	auto post_req = MakePost(R"({"action":"start","filename":"x.cap"})");
	auto post_resp = InvokeRoute(route, post_req);
	BOOST_CHECK_EQUAL(boost::beast::http::status::service_unavailable, post_resp.result());
}

BOOST_AUTO_TEST_SUITE_END()
