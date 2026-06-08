#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <boost/asio/io_context.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url_view.hpp>

#include "mocks/mock_beast_basicstream_with_timeout.h"
#include "http/server/responses/response_400.h"
#include "http/server/responses/response_404.h"
#include "http/server/responses/response_staticfile.h"
#include "http/server/server_types.h"
#include "http/server/static_file_handler.h"

using namespace AqualinkAutomate::HTTP;

namespace
{

	// RAII fixture creating an isolated temporary document-root tree:
	//
	//   <base>/docroot/index.html
	//   <base>/docroot/sub/page.html
	//   <base>/docrootsecret/secret.txt   (SIBLING that shares the "docroot" prefix)
	//
	// The sibling directory deliberately begins with the same string as the served
	// document root so a raw string-prefix path jail would wrongly admit it.
	struct StaticFileFixture
	{
		StaticFileFixture()
		{
			static unsigned int counter = 0;
			auto unique = std::string{ "aqualink_sfh_test_" } + std::to_string(counter++);
			base = std::filesystem::temp_directory_path() / unique;

			// Start from a clean slate in case a previous run left the tree behind.
			std::error_code rm_ec;
			std::filesystem::remove_all(base, rm_ec);

			doc_root = base / "docroot";
			sibling = base / "docrootsecret";

			std::filesystem::create_directories(doc_root / "sub");
			std::filesystem::create_directories(sibling);

			WriteFile(doc_root / "index.html", "<html>root index</html>");
			WriteFile(doc_root / "sub" / "page.html", "<html>sub page</html>");
			WriteFile(sibling / "secret.txt", "TOP SECRET");
		}

		~StaticFileFixture()
		{
			std::error_code ec;
			std::filesystem::remove_all(base, ec);
		}

		static void WriteFile(const std::filesystem::path& path, std::string_view content)
		{
			std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
			ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
		}

		std::filesystem::path base;
		std::filesystem::path doc_root;
		std::filesystem::path sibling;
	};

	Request MakeRequest(std::string_view target)
	{
		Request req;
		req.version(11);
		req.method(boost::beast::http::verb::get);
		req.target(boost::beast::string_view{ target.data(), target.size() });
		return req;
	}

	bool RunMatch(StaticFileHandler& handler, std::string_view target, std::filesystem::path& result)
	{
		auto parsed = boost::urls::parse_uri_reference(target);
		BOOST_REQUIRE(!parsed.has_error());
		return handler.match(parsed.value(), result);
	}

	// Serialize the type-erased HTTP::Message (message_generator) back into an
	// inspectable string_body response by writing it through a pair of connected
	// in-memory beast test streams and reading it back, mirroring the existing
	// PerformHttpRequestResponse support helper.
	HTTP::Response SerializeMessage(HTTP::Message&& msg)
	{
		boost::asio::io_context ioc;
		auto exec = ioc.get_executor();

		AqualinkAutomate::Test::MockBeastBasicStreamWithTimeout client_stream(exec);
		AqualinkAutomate::Test::MockBeastBasicStreamWithTimeout server_stream(exec);
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

}
// unnamed namespace

BOOST_AUTO_TEST_SUITE(TestSuite_HttpStaticFile)

//----------------------------------------------------------------------------
//  Reflected-XSS escaping in error bodies (Response_400 / Response_404)
//----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_StaticFile_Response404_EscapesReflectedTarget)
{
	auto req = MakeRequest("/<script>alert(1)</script>");

	auto res = Responses::Response_404(req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::not_found, res.result());
	// The raw, unescaped markup must NOT appear verbatim in the body.
	BOOST_CHECK(res.body().find("<script>") == std::string::npos);
	// The escaped form must appear instead.
	BOOST_CHECK(res.body().find("&lt;script&gt;") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(Test_StaticFile_Response400_EscapesReflectedTarget)
{
	auto req = MakeRequest("/\"><img src=x onerror=alert(1)>");

	auto res = Responses::Response_400(req);

	BOOST_CHECK_EQUAL(boost::beast::http::status::bad_request, res.result());
	BOOST_CHECK(res.body().find("<img") == std::string::npos);
	BOOST_CHECK(res.body().find("&lt;img") != std::string::npos);
	// Double-quote that could break out of an attribute context is escaped too.
	BOOST_CHECK(res.body().find("&quot;") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(Test_StaticFile_Response404_PlainTargetUnchanged)
{
	auto req = MakeRequest("/missing/resource");

	auto res = Responses::Response_404(req);

	// A benign target with no HTML-significant characters is reflected verbatim.
	BOOST_CHECK(res.body().find("/missing/resource") != std::string::npos);
}

//----------------------------------------------------------------------------
//  Static-file path jail (StaticFileHandler::match)
//----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_StaticFile_Jail_AllowsLegitimateAsset, StaticFileFixture)
{
	StaticFileHandler handler("/", doc_root);

	std::filesystem::path result;
	BOOST_CHECK(RunMatch(handler, "/sub/page.html", result));
	BOOST_CHECK_EQUAL(result.filename().string(), "page.html");
}

BOOST_FIXTURE_TEST_CASE(Test_StaticFile_Jail_RejectsParentTraversal, StaticFileFixture)
{
	StaticFileHandler handler("/", doc_root);

	std::filesystem::path result;
	// Escaping out of the root into the parent directory via ".." must be
	// rejected (no string-prefix collision here -- a plain parent escape).
	BOOST_CHECK(!RunMatch(handler, "/../secret.txt", result));
}

BOOST_FIXTURE_TEST_CASE(Test_StaticFile_Jail_RejectsSiblingPrefixEscape, StaticFileFixture)
{
	// REGRESSION: the previous jail used a raw string-prefix compare, so a
	// resolved path of "<base>/docrootsecret/secret.txt" passed because it
	// textually starts with the root "<base>/docroot".  Reaching that sibling
	// from the served root "<base>/docroot" requires a single "..", and the
	// canonicalised result still shares the root's string prefix -- exactly the
	// case the old compare admitted.  The component-boundary check must reject it.
	StaticFileHandler handler("/", doc_root);

	std::filesystem::path result;
	BOOST_CHECK(!RunMatch(handler, "/../docrootsecret/secret.txt", result));
}

//----------------------------------------------------------------------------
//  Static-file ETag / conditional GET (Response_StaticFile)
//----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_StaticFile_EmitsETagOn200, StaticFileFixture)
{
	auto req = MakeRequest("/index.html");

	auto res = SerializeMessage(Responses::Response_StaticFile(req, doc_root / "index.html"));

	BOOST_CHECK_EQUAL(boost::beast::http::status::ok, res.result());

	auto etag_it = res.find(boost::beast::http::field::etag);
	BOOST_REQUIRE(etag_it != res.end());
	BOOST_CHECK(!etag_it->value().empty());
	// The 200 carries the actual file content.
	BOOST_CHECK(res.body().find("root index") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(Test_StaticFile_ConditionalGetReturns304, StaticFileFixture)
{
	const auto file = doc_root / "index.html";

	// First request: capture the served ETag.
	auto first_req = MakeRequest("/index.html");
	auto first = SerializeMessage(Responses::Response_StaticFile(first_req, file));

	auto etag_it = first.find(boost::beast::http::field::etag);
	BOOST_REQUIRE(etag_it != first.end());
	const std::string etag{ etag_it->value().data(), etag_it->value().size() };

	// Second request: present the ETag back via If-None-Match -> expect 304 with
	// no body re-sent.
	auto cond_req = MakeRequest("/index.html");
	cond_req.set(boost::beast::http::field::if_none_match, etag);

	auto cond = SerializeMessage(Responses::Response_StaticFile(cond_req, file));
	BOOST_CHECK_EQUAL(boost::beast::http::status::not_modified, cond.result());
	BOOST_CHECK(cond.body().empty());
}

BOOST_AUTO_TEST_SUITE_END()
