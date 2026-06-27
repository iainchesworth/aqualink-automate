#include <boost/test/unit_test.hpp>

#include "http/server/responses/response_400.h"
#include "http/server/responses/response_404.h"
#include "http/server/responses/response_405.h"
#include "http/server/responses/response_500.h"
#include "http/server/responses/response_503.h"

using namespace AqualinkAutomate::HTTP;

BOOST_AUTO_TEST_SUITE(TestSuite_HttpResponses)

BOOST_AUTO_TEST_CASE(Test_HttpResponses_4xx)
{
	Request req;

	req.version(11);
	req.method(boost::beast::http::verb::get);
	req.target("/error-4xx-response-unit-test");

	auto res400(Responses::Response_400(req));
	BOOST_CHECK_EQUAL(boost::beast::http::status::bad_request, res400.result());

	auto res404 = Responses::Response_404(req);
	BOOST_CHECK_EQUAL(boost::beast::http::status::not_found, res404.result());

	auto res405 = Responses::Response_405(req);
	BOOST_CHECK_EQUAL(boost::beast::http::status::method_not_allowed, res405.result());
}

BOOST_AUTO_TEST_CASE(Test_HttpResponses_4xx_ReflectedTargetIsHtmlEscaped)
{
	// Regression (reflected XSS): the request target is client-controlled and is
	// reflected into the text/html error body. Response_400/404 escaped it but
	// Response_405 did not, so a path-parameter route hit with a non-GET method
	// reflected raw markup. All three must HTML-escape the target now: the raw
	// "<svg onload=..." must not appear; the escaped "&lt;svg" must.
	Request req;
	req.version(11);
	req.method(boost::beast::http::verb::put);
	// Slash-free payload so it stays a single path segment (matches a {param} route).
	req.target("/api/schedules/<svg onload=alert(1)>");

	const std::string raw{ "<svg onload=alert(1)>" };

	auto res400 = Responses::Response_400(req);
	BOOST_CHECK(res400.body().find(raw) == std::string::npos);
	BOOST_CHECK(res400.body().find("&lt;svg") != std::string::npos);

	auto res404 = Responses::Response_404(req);
	BOOST_CHECK(res404.body().find(raw) == std::string::npos);
	BOOST_CHECK(res404.body().find("&lt;svg") != std::string::npos);

	auto res405 = Responses::Response_405(req);
	BOOST_CHECK(res405.body().find(raw) == std::string::npos);
	BOOST_CHECK(res405.body().find("&lt;svg") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(Test_HttpResponses_5xx)
{
	Request req;

	req.version(11);
	req.method(boost::beast::http::verb::get);
	req.target("/error-5xx-response-unit-test");

	auto res500_1 = Responses::Response_500(req);
	BOOST_CHECK_EQUAL(boost::beast::http::status::internal_server_error, res500_1.result());
	BOOST_CHECK_EQUAL("Internal Server Error: Unknown", res500_1.body());

	auto res500_2 = Responses::Response_500(req, std::nullopt);
	BOOST_CHECK_EQUAL(boost::beast::http::status::internal_server_error, res500_2.result());
	BOOST_CHECK_EQUAL("Internal Server Error: Unknown", res500_2.body());

	auto res500_3 = Responses::Response_500(req, std::string_view{ "THIS IS A TEST" });
	BOOST_CHECK_EQUAL(boost::beast::http::status::internal_server_error, res500_3.result());
	BOOST_CHECK_EQUAL("Internal Server Error: THIS IS A TEST", res500_3.body());

	auto res503_1 = Responses::Response_503(req);
	BOOST_CHECK_EQUAL(boost::beast::http::status::service_unavailable, res503_1.result());
	BOOST_CHECK_EQUAL("Service Unavailable: Unknown", res503_1.body());

	auto res503_2 = Responses::Response_503(req, std::nullopt);
	BOOST_CHECK_EQUAL(boost::beast::http::status::service_unavailable, res503_2.result());
	BOOST_CHECK_EQUAL("Service Unavailable: Unknown", res503_2.body());

	auto res503_3 = Responses::Response_503(req, std::string_view{ "THIS IS A TEST" });
	BOOST_CHECK_EQUAL(boost::beast::http::status::service_unavailable, res503_3.result());
	BOOST_CHECK_EQUAL("Service Unavailable: THIS IS A TEST", res503_3.body());
}

BOOST_AUTO_TEST_SUITE_END()
