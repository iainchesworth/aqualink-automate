#include <string>
#include <string_view>

#include <boost/test/unit_test.hpp>

#include "http/server/server_fields.h"
#include "version/version_cmake.h"

using namespace AqualinkAutomate::HTTP;

BOOST_AUTO_TEST_SUITE(HttpServerFields_TestSuite)

// =============================================================================
// ContentTypes
// =============================================================================

BOOST_AUTO_TEST_CASE(TestContentTypes_ApplicationJson)
{
	BOOST_CHECK_EQUAL(ContentTypes::APPLICATION_JSON, "application/json");
}

BOOST_AUTO_TEST_CASE(TestContentTypes_TextHtml)
{
	BOOST_CHECK_EQUAL(ContentTypes::TEXT_HTML, "text/html");
}

BOOST_AUTO_TEST_CASE(TestContentTypes_TextPlain)
{
	BOOST_CHECK_EQUAL(ContentTypes::TEXT_PLAIN, "text/plain");
}

// =============================================================================
// ServerFields
// =============================================================================

BOOST_AUTO_TEST_CASE(TestRetryAfter_Returns30)
{
	auto retry = ServerFields::RetryAfter();
	BOOST_CHECK_EQUAL(retry, "30");
}

BOOST_AUTO_TEST_CASE(TestRetryAfter_ConsistentAcrossCalls)
{
	auto first = ServerFields::RetryAfter();
	auto second = ServerFields::RetryAfter();
	BOOST_CHECK_EQUAL(first, second);
}

BOOST_AUTO_TEST_CASE(TestServer_ContainsProjectName)
{
	auto server = ServerFields::Server();
	auto project_name = AqualinkAutomate::Version::VersionInfo::ProjectName();
	BOOST_CHECK(server.find(project_name) != std::string_view::npos);
}

BOOST_AUTO_TEST_CASE(TestServer_ContainsVersion)
{
	auto server = ServerFields::Server();
	auto version = AqualinkAutomate::Version::VersionInfo::ProjectVersion();
	BOOST_CHECK(server.find(version) != std::string_view::npos);
}

BOOST_AUTO_TEST_CASE(TestServer_ContainsSlashSeparator)
{
	auto server = ServerFields::Server();
	BOOST_CHECK(server.find('/') != std::string_view::npos);
}

BOOST_AUTO_TEST_CASE(TestServer_ConsistentAcrossCalls)
{
	auto first = ServerFields::Server();
	auto second = ServerFields::Server();
	BOOST_CHECK_EQUAL(first, second);
}

BOOST_AUTO_TEST_SUITE_END()
