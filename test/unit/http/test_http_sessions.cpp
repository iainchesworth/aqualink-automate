#include <array>
#include <memory>
#include <string>
#include <string_view>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/test/unit_test.hpp>

#include "http/webroute_page_index.h"
#include "http/webroute_page_equipment.h"
#include "http/webroute_page_version.h"
#include "http/server/http_plainsession.h"
#include "http/server/routing/routing.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"

#include "mocks/mock_beast_basicstream_with_timeout.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::HTTP;
using namespace AqualinkAutomate::Test;

BOOST_AUTO_TEST_SUITE(TestSuite_HttpSessions)

BOOST_AUTO_TEST_CASE(Test_HttpSessions_PlainSession_RenderedPages)
{
	using HTTP_PlainSession_Test = HTTP_PlainSession_Base<Test::MockBeastBasicStreamWithTimeout>;

	boost::asio::io_context io_context;

	Kernel::HubLocator hub_locator;
	hub_locator.Register(std::make_shared<Kernel::DataHub>());

	Routing::Add(std::make_unique<WebRoute_Page_Index>(hub_locator));
	Routing::Add(std::make_unique<WebRoute_Page_Equipment>(hub_locator));
	Routing::Add(std::make_unique<WebRoute_Page_Version>());

	/// TEST API ROUTE: /

	{
		Test::MockBeastBasicStreamWithTimeout test_stream_remote(io_context), test_stream_local(io_context);
		test_stream_local.connect(test_stream_remote);

		boost::beast::http::request<boost::beast::http::string_body> req;
		req.version(11);
		req.method(boost::beast::http::verb::get);
		req.target("/");
		req.set(boost::beast::http::field::host, "localhost.localdomain");
		req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

		boost::beast::flat_buffer test_buffer_remote;
		auto session = std::make_shared<HTTP_PlainSession_Test>(std::move(test_stream_remote), std::move(test_buffer_remote));
		BOOST_TEST_REQUIRE(nullptr != session);

		session->Run();

		boost::beast::error_code ec;

		const auto bytes_written = boost::beast::http::write(test_stream_local, req, ec);
		BOOST_TEST_REQUIRE(0 == ec.value());
		BOOST_TEST_REQUIRE(0 != bytes_written);

		io_context.poll_one(); // ASYNC_READ (request)
		io_context.poll_one(); // ASYNC_WRITE (response)
		io_context.poll_one(); // ASYNC_READ (queued)

		boost::beast::flat_buffer buffer;
		boost::beast::http::response<boost::beast::http::string_body> res;

		const auto bytes_read = boost::beast::http::read(test_stream_local, buffer, res, ec);
		BOOST_TEST_REQUIRE(0 == ec.value());
		BOOST_TEST_REQUIRE(0 != bytes_read);
		BOOST_CHECK_EQUAL(boost::beast::http::status::ok, res.result());

		const auto res_body = res.body();
		BOOST_TEST_REQUIRE(0 != res_body.length());
	}

	/// TEST API ROUTE: /equipment

	{
		Test::MockBeastBasicStreamWithTimeout test_stream_remote(io_context), test_stream_local(io_context);
		test_stream_local.connect(test_stream_remote);

		boost::beast::http::request<boost::beast::http::string_body> req;
		req.version(11);
		req.method(boost::beast::http::verb::get);
		req.target("/equipment");
		req.set(boost::beast::http::field::host, "localhost.localdomain");
		req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

		boost::beast::flat_buffer test_buffer_remote;
		auto session = std::make_shared<HTTP_PlainSession_Test>(std::move(test_stream_remote), std::move(test_buffer_remote));
		BOOST_TEST_REQUIRE(nullptr != session);

		session->Run();

		boost::beast::error_code ec;

		const auto bytes_written = boost::beast::http::write(test_stream_local, req, ec);
		BOOST_TEST_REQUIRE(0 == ec.value());
		BOOST_TEST_REQUIRE(0 != bytes_written);

		io_context.poll_one(); // ASYNC_READ (request)
		io_context.poll_one(); // ASYNC_WRITE (response)
		io_context.poll_one(); // ASYNC_READ (queued)

		boost::beast::flat_buffer buffer;
		boost::beast::http::response<boost::beast::http::string_body> res;

		const auto bytes_read = boost::beast::http::read(test_stream_local, buffer, res, ec);
		BOOST_TEST_REQUIRE(0 == ec.value());
		BOOST_TEST_REQUIRE(0 != bytes_read);
		BOOST_CHECK_EQUAL(boost::beast::http::status::ok, res.result());

		const auto res_body = res.body();
		BOOST_TEST_REQUIRE(0 != res_body.length());
	}

	/// TEST API ROUTE: /version

	{
		Test::MockBeastBasicStreamWithTimeout test_stream_remote(io_context), test_stream_local(io_context);
		test_stream_local.connect(test_stream_remote);

		boost::beast::http::request<boost::beast::http::string_body> req;
		req.version(11);
		req.method(boost::beast::http::verb::get);
		req.target("/version");
		req.set(boost::beast::http::field::host, "localhost.localdomain");
		req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

		boost::beast::flat_buffer test_buffer_remote;
		auto session = std::make_shared<HTTP_PlainSession_Test>(std::move(test_stream_remote), std::move(test_buffer_remote));
		BOOST_TEST_REQUIRE(nullptr != session);

		session->Run();

		boost::beast::error_code ec;

		const auto bytes_written = boost::beast::http::write(test_stream_local, req, ec);
		BOOST_TEST_REQUIRE(0 == ec.value());
		BOOST_TEST_REQUIRE(0 != bytes_written);

		io_context.poll_one(); // ASYNC_READ (request)
		io_context.poll_one(); // ASYNC_WRITE (response)
		io_context.poll_one(); // ASYNC_READ (queued)

		boost::beast::flat_buffer buffer;
		boost::beast::http::response<boost::beast::http::string_body> res;

		const auto bytes_read = boost::beast::http::read(test_stream_local, buffer, res, ec);
		BOOST_TEST_REQUIRE(0 == ec.value());
		BOOST_TEST_REQUIRE(0 != bytes_read);
		BOOST_CHECK_EQUAL(boost::beast::http::status::ok, res.result());

		const auto res_body = res.body();
		BOOST_TEST_REQUIRE(0 != res_body.length());
	}
}

BOOST_AUTO_TEST_SUITE_END()
