#include <string>

#include <boost/test/unit_test.hpp>

#include "http/server/make_response.h"
#include "http/server/server_fields.h"

using namespace AqualinkAutomate::HTTP;

BOOST_AUTO_TEST_SUITE(TestSuite_HttpMakeResponse)

namespace
{
	Request MakeRequest(bool keep_alive)
	{
		Request req;
		req.version(11);
		req.method(boost::beast::http::verb::get);
		req.target("/make-response-unit-test");
		req.keep_alive(keep_alive);
		return req;
	}
}

BOOST_AUTO_TEST_CASE(Test_MakeResponse_PopulatesStatusBodyAndHeaders)
{
	const auto req = MakeRequest(true);

	auto resp = MakeResponse(req, Status::ok, ContentTypes::TEXT_PLAIN, "hello world");

	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());
	BOOST_CHECK_EQUAL("hello world", resp.body());
	BOOST_CHECK_EQUAL(ContentTypes::TEXT_PLAIN, resp[boost::beast::http::field::content_type]);
	BOOST_CHECK_EQUAL(ServerFields::Server(), resp[boost::beast::http::field::server]);

	// prepare_payload() must have been called: Content-Length reflects the body size.
	BOOST_CHECK_EQUAL(std::string("11"), std::string(resp[boost::beast::http::field::content_length]));
}

BOOST_AUTO_TEST_CASE(Test_MakeResponse_MirrorsKeepAlive)
{
	{
		const auto req = MakeRequest(true);
		auto resp = MakeResponse(req, Status::ok, ContentTypes::TEXT_PLAIN, "x");
		BOOST_CHECK(resp.keep_alive());
	}

	{
		const auto req = MakeRequest(false);
		auto resp = MakeResponse(req, Status::ok, ContentTypes::TEXT_PLAIN, "x");
		BOOST_CHECK(!resp.keep_alive());
	}
}

BOOST_AUTO_TEST_CASE(Test_MakeResponse_PropagatesNonOkStatus)
{
	const auto req = MakeRequest(true);

	auto resp = MakeResponse(req, Status::not_found, ContentTypes::TEXT_PLAIN, "missing");

	BOOST_CHECK_EQUAL(boost::beast::http::status::not_found, resp.result());
	BOOST_CHECK_EQUAL("missing", resp.body());
}

BOOST_AUTO_TEST_CASE(Test_MakeJsonResponse_SetsJsonContentType)
{
	const auto req = MakeRequest(true);

	auto resp = MakeJsonResponse(req, Status::ok, R"({"status":"ok"})");

	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, resp.result());
	BOOST_CHECK_EQUAL(R"({"status":"ok"})", resp.body());
	BOOST_CHECK_EQUAL(ContentTypes::APPLICATION_JSON, resp[boost::beast::http::field::content_type]);
	BOOST_CHECK_EQUAL(ServerFields::Server(), resp[boost::beast::http::field::server]);
}

BOOST_AUTO_TEST_CASE(Test_MakeJsonResponse_PropagatesErrorStatus)
{
	const auto req = MakeRequest(true);

	auto resp = MakeJsonResponse(req, Status::bad_request, R"({"error":"bad"})");

	BOOST_CHECK_EQUAL(boost::beast::http::status::bad_request, resp.result());
	BOOST_CHECK_EQUAL(R"({"error":"bad"})", resp.body());
	BOOST_CHECK_EQUAL(ContentTypes::APPLICATION_JSON, resp[boost::beast::http::field::content_type]);
}

BOOST_AUTO_TEST_SUITE_END()
