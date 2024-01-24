#include <memory>

#include <boost/asio/io_context.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/_experimental/test/handler.hpp>
#include <boost/test/unit_test.hpp>

#include "http/server/http_plainsession.h"
#include "http/server/websocket_plainsession.h"

#include "mocks/mock_beast_basicstream_with_timeout.h"
#include "support/unit_test_httprequestresponse.h"

using namespace AqualinkAutomate;

namespace AqualinkAutomate::Test
{

	std::unique_ptr<TestResponseObject> PerformHttpRequestResponse(boost::asio::io_context& io_context, const std::string_view& url_to_retrieve)
	{
		boost::beast::flat_buffer remote_stream_buffer;
		Test::MockBeastBasicStreamWithTimeout remote_stream(io_context);
		auto tro = std::make_unique<Test::TestResponseObject>(io_context, remote_stream);

		tro->last_request.version(11);
		tro->last_request.method(boost::beast::http::verb::get);
		tro->last_request.target(url_to_retrieve);
		tro->last_request.set(boost::beast::http::field::host, "localhost.localdomain");
		tro->last_request.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

		tro->session_ptr = std::make_shared<HTTP_PlainSession_Test>(std::move(remote_stream), std::move(remote_stream_buffer));
		BOOST_TEST_REQUIRE(nullptr != tro->session_ptr);

		tro->session_ptr->Run();

		boost::beast::error_code ec;

		const auto bytes_written = boost::beast::http::write(tro->local_stream, tro->last_request, ec);
		BOOST_TEST_REQUIRE(0 == ec.value());
		BOOST_TEST_REQUIRE(0 != bytes_written);

		io_context.poll_one(); // ASYNC_READ (request)
		io_context.poll_one(); // ASYNC_WRITE (response)
		io_context.poll_one(); // ASYNC_READ (queued)

		const auto bytes_read = boost::beast::http::read(tro->local_stream, tro->response_buffer, tro->last_response, ec);

		return std::move(tro);
	}

	std::unique_ptr<TestResponseObject> PerformHttpWsUpgradeResponse(boost::asio::io_context& io_context, const std::string_view& url_to_retrieve)
	{
		boost::beast::flat_buffer remote_stream_buffer;
		Test::MockBeastBasicStreamWithTimeout remote_stream(io_context);
		auto tro = std::make_unique<Test::TestResponseObject>(io_context, remote_stream);

		tro->last_request.version(11);
		tro->last_request.method(boost::beast::http::verb::get);
		tro->last_request.target(url_to_retrieve);
		tro->last_request.set(boost::beast::http::field::host, "localhost.localdomain");
		tro->last_request.set(boost::beast::http::field::connection, "upgrade");
		tro->last_request.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
		tro->last_request.set(boost::beast::http::field::upgrade, "websocket");
		tro->last_request.set(boost::beast::http::field::origin, "ws://localhost.localdomain/");
		tro->last_request.set(boost::beast::http::field::sec_websocket_key, "x3JJHMbDL1EzLkh9GBhXDw==");
		tro->last_request.set(boost::beast::http::field::sec_websocket_version, "13");

		tro->session_ptr = std::make_shared<HTTP_PlainSession_Test>(std::move(remote_stream), std::move(remote_stream_buffer));
		BOOST_TEST_REQUIRE(nullptr != tro->session_ptr);

		tro->session_ptr->Run();

		boost::beast::error_code ec;
		
		const auto bytes_written = boost::beast::http::write(tro->local_stream, tro->last_request, ec);
		BOOST_TEST_REQUIRE(0 == ec.value());
		BOOST_TEST_REQUIRE(0 != bytes_written);

		io_context.poll_one(); // ASYNC_READ
		io_context.poll_one(); // ASYNC_ACCEPT
		
		const auto bytes_read = boost::beast::http::read(tro->local_stream, tro->response_buffer, tro->last_response, ec);
		BOOST_TEST_REQUIRE(0 == ec.value());
		BOOST_TEST_REQUIRE(boost::beast::http::status::switching_protocols == tro->last_response.result());

		return std::move(tro);
	}

}
// namespace AqualinkAutomate::Test
