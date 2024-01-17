#include <array>
#include <memory>
#include <string>
#include <string_view>

#include <boost/beast/_experimental/test/stream.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/test/unit_test.hpp>

#include "http/webroute_page_version.h"
#include "http/server/http_plainsession.h"
#include "http/server/routing/routing.h"

using namespace AqualinkAutomate::HTTP;

BOOST_AUTO_TEST_SUITE(TestSuite_HttpSessions)

BOOST_AUTO_TEST_CASE(Test_HttpSessions_PlainSession)
{
	using HTTP_PlainSession_Test = HTTP_PlainSession_Base<boost::beast::test::stream>;

	Routing::Add(std::make_unique<WebRoute_Page_Version>()); /// <root url>/version
	const std::string_view test_http_get_version{ "GET /version HTTP/1.1\nHost: localhost.localdomain\nUser-Agent: AqualinkAutomateTestAgent/1.0\nAccept: *\n\n" };

	boost::asio::io_context io_context;
	boost::beast::test::stream test_stream_remote(io_context), test_stream_local(io_context);
	boost::beast::flat_buffer test_buffer_remote;
	std::array<char, 1024> test_buffer_local;
		
	test_stream_local.connect(test_stream_remote);

	auto session = std::make_shared<HTTP_PlainSession_Test>(std::move(test_stream_remote), std::move(test_buffer_remote));
	BOOST_TEST_REQUIRE(nullptr != session);

	const auto bytes_written = test_stream_local.write_some(boost::asio::buffer(test_http_get_version));
	BOOST_TEST_REQUIRE(test_http_get_version.size() <= bytes_written);

	const auto bytes_read = test_stream_local.read_some(boost::asio::buffer(test_buffer_local));
	BOOST_CHECK_NE(0, bytes_read);
	BOOST_CHECK_NE(0, test_buffer_local.size());
}

BOOST_AUTO_TEST_SUITE_END()
