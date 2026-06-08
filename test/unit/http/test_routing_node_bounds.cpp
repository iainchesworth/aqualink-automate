#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/url/parse_path.hpp>

#include "http/server/routing/matches.h"
#include "http/server/routing/node.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::HTTP;

// These tests pin the behaviour of the bounds-checked capture path added to
// node.h: find_impl/try_match/find_optional_resource now thread matches_end /
// ids_end pointers and route every capture write through WriteCapture so a route
// template can never write past the caller's fixed-size capture arrays. The
// checks here confirm that supplying the end pointers does NOT regress matching
// for any of the supported segment kinds (literal / var / optional / star /
// plus) and that captured values are still correct.

BOOST_AUTO_TEST_SUITE(TestSuite_HTTPServer_RoutingNodeBounds)

namespace
{
	class Handler {};
}

BOOST_AUTO_TEST_CASE(Test_NodeBounds_AllSegmentKinds_WithEndPointers)
{
	std::vector<std::tuple<std::string, std::shared_ptr<Handler>>> routes
	{
		{ "/none", std::make_shared<Handler>() },
		{ "/var/{path}", std::make_shared<Handler>() },
		{ "/optional/{path?}", std::make_shared<Handler>() },
		{ "/star/{path*}", std::make_shared<Handler>() },
		{ "/plus/{path+}", std::make_shared<Handler>() }
	};

	Routing::impl<Handler> tree;
	for (auto& [route, handler] : routes)
	{
		tree.insert_impl(route, handler.get());
	}

	auto find = [&tree](std::string_view target, Routing::matches& m) -> Handler*
	{
		std::string_view* matches_it = m.matches();
		std::string_view* ids_it = m.ids();
		std::string_view* matches_end = m.matches() + m.size();
		std::string_view* ids_end = m.ids() + m.size();

		auto path = boost::urls::parse_path(target);
		BOOST_TEST_REQUIRE(!path.has_error());
		return tree.find_impl(*path, matches_it, ids_it, matches_end, ids_end);
	};

	{
		Routing::matches m;
		BOOST_CHECK_NE(nullptr, find("/none", m));
	}

	{
		Routing::matches m;
		BOOST_CHECK_NE(nullptr, find("/var/one", m));
		std::string captured;
		boost::urls::pct_string_view(m[0]).decode({}, boost::urls::string_token::append_to(captured));
		BOOST_CHECK_EQUAL("one", captured);
	}

	{
		Routing::matches m;
		BOOST_CHECK_NE(nullptr, find("/optional", m));
	}

	{
		Routing::matches m;
		BOOST_CHECK_NE(nullptr, find("/star/a/b/c", m));
		std::string captured;
		boost::urls::pct_string_view(m[0]).decode({}, boost::urls::string_token::append_to(captured));
		BOOST_CHECK_EQUAL("a/b/c", captured);
	}

	{
		Routing::matches m;
		BOOST_CHECK_EQUAL(nullptr, find("/plus", m));   // {path+} requires >=1 segment
		BOOST_CHECK_NE(nullptr, find("/plus/x", m));
	}
}

// A long star-captured path must match correctly when the capture array is large
// enough to hold every replacement field; the end-pointer guard only engages on
// genuine overflow, which the registered routes never reach.
BOOST_AUTO_TEST_CASE(Test_NodeBounds_DeepStarPath_WithinCapacity)
{
	Routing::impl<Handler> tree;
	auto handler = std::make_shared<Handler>();
	tree.insert_impl("/files/{path*}", handler.get());

	Routing::matches m;   // matches_storage<20>
	std::string_view* matches_it = m.matches();
	std::string_view* ids_it = m.ids();
	std::string_view* matches_end = m.matches() + m.size();
	std::string_view* ids_end = m.ids() + m.size();

	auto path = boost::urls::parse_path("/files/a/b/c/d/e/f/g/h");
	BOOST_TEST_REQUIRE(!path.has_error());
	auto p = tree.find_impl(*path, matches_it, ids_it, matches_end, ids_end);
	BOOST_CHECK_NE(nullptr, p);

	std::string captured;
	boost::urls::pct_string_view(m[0]).decode({}, boost::urls::string_token::append_to(captured));
	BOOST_CHECK_EQUAL("a/b/c/d/e/f/g/h", captured);
}

// The legacy (no-end-pointer) overload must still compile and behave as before,
// keeping existing callers source-compatible.
BOOST_AUTO_TEST_CASE(Test_NodeBounds_LegacyOverload_StillWorks)
{
	Routing::impl<Handler> tree;
	auto handler = std::make_shared<Handler>();
	tree.insert_impl("/legacy/{id}", handler.get());

	Routing::matches m;
	std::string_view* matches_it = m.matches();
	std::string_view* ids_it = m.ids();

	auto path = boost::urls::parse_path("/legacy/42");
	BOOST_TEST_REQUIRE(!path.has_error());
	// No end pointers supplied (defaults to nullptr -> unchecked legacy behaviour).
	auto p = tree.find_impl(*path, matches_it, ids_it);
	BOOST_CHECK_NE(nullptr, p);

	std::string captured;
	boost::urls::pct_string_view(m[0]).decode({}, boost::urls::string_token::append_to(captured));
	BOOST_CHECK_EQUAL("42", captured);
}

BOOST_AUTO_TEST_SUITE_END()
