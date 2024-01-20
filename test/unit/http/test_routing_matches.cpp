#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/url/parse_path.hpp>

#include "http/server/routing/matches.h"
#include "http/server/routing/node.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::HTTP;

BOOST_AUTO_TEST_SUITE(TestSuite_HTTPServer_RoutingMatches)

class MyTestClass
{
};

BOOST_AUTO_TEST_CASE(TestSuite_HTTPServer_RoutingMatches_PageRoutes)
{
	std::vector<std::tuple<std::string, std::shared_ptr<MyTestClass>>> test_routes_vec
	{
		{ "/", std::make_shared<MyTestClass>() },
		{ "/equipment", std::make_shared<MyTestClass>() },
		{ "/version", std::make_shared<MyTestClass>() }
	};

	Routing::impl<MyTestClass> test_routes;

	for (auto& [route, handler] : test_routes_vec)
	{
		test_routes.insert_impl(route, handler.get());
	}

	for (auto& [route, _] : test_routes_vec)
	{
		Routing::matches m;

		std::string_view* matches_it = m.matches();
		std::string_view* ids_it = m.ids();

		auto path = boost::urls::parse_path(route);
		BOOST_TEST_REQUIRE(!path.has_error());
		auto p = test_routes.find_impl(*path, matches_it, ids_it);
		BOOST_CHECK_NE(nullptr, p);
	}
}

BOOST_AUTO_TEST_CASE(TestSuite_HTTPServer_RoutingMatches_Tests)
{
	std::vector<std::tuple<std::string, std::shared_ptr<MyTestClass>>> test_routes_vec
	{
		{ "/test/path/1", std::make_shared<MyTestClass>() },
		{ "/test/path/2", std::make_shared<MyTestClass>() },
		{ "/test/path/3", std::make_shared<MyTestClass>() },
		{ "/really/long/test/path/three", std::make_shared<MyTestClass>() },
		{ "/static/index.html", std::make_shared<MyTestClass>() }
	};

	Routing::impl<MyTestClass> test_routes;

	for (auto& [route, handler] : test_routes_vec)
	{
		test_routes.insert_impl(route, handler.get());
	}
	
	for (auto& [route, _] : test_routes_vec)
	{
		Routing::matches m;

		std::string_view* matches_it = m.matches();
		std::string_view* ids_it = m.ids();

		auto path = boost::urls::parse_path(route);
		BOOST_TEST_REQUIRE(!path.has_error());
		auto p = test_routes.find_impl(*path, matches_it, ids_it);
		BOOST_CHECK_NE(nullptr, p);
	}
}

BOOST_AUTO_TEST_CASE(TestSuite_HTTPServer_RoutingMatches_CustomRouteMatches)
{
	std::vector<std::tuple<std::string, std::shared_ptr<MyTestClass>>> test_routes_vec
	{
		{ "/none", std::make_shared<MyTestClass>() },
		{ "/var/{path}", std::make_shared<MyTestClass>() },
		{ "/optional/{path?}", std::make_shared<MyTestClass>() },
		{ "/star/{path*}", std::make_shared<MyTestClass>() },
		{ "/plus/{path+}", std::make_shared<MyTestClass>() }
	};

	Routing::impl<MyTestClass> test_routes;

	for (auto& [route, handler] : test_routes_vec)
	{
		test_routes.insert_impl(route, handler.get());
	}

	{
		Routing::matches m;
		std::string_view* matches_it = m.matches();
		std::string_view* ids_it = m.ids();

		auto none = boost::urls::parse_path("/none");
		BOOST_TEST_REQUIRE(!none.has_error());
		auto p = test_routes.find_impl(*none, matches_it, ids_it);
		BOOST_CHECK_NE(nullptr, p);
	}

	{
		Routing::matches m;
		std::string_view* matches_it = m.matches();
		std::string_view* ids_it = m.ids();

		auto var0 = boost::urls::parse_path("/var");
		BOOST_TEST_REQUIRE(!var0.has_error());
		auto p0 = test_routes.find_impl(*var0, matches_it, ids_it);
		BOOST_CHECK_EQUAL(nullptr, p0);

		auto var1 = boost::urls::parse_path("/var/one");
		BOOST_TEST_REQUIRE(!var1.has_error());
		auto p1 = test_routes.find_impl(*var1, matches_it, ids_it);
		BOOST_CHECK_NE(nullptr, p1);

		std::string var1_msg{ "" };
		boost::urls::pct_string_view(m[0]).decode({}, boost::urls::string_token::append_to(var1_msg));
		BOOST_CHECK_EQUAL("one", var1_msg);

		auto var2 = boost::urls::parse_path("/var/two");
		BOOST_TEST_REQUIRE(!var2.has_error());
		auto p2 = test_routes.find_impl(*var2, matches_it, ids_it);
		BOOST_CHECK_NE(nullptr, p2);

		std::string var2_msg{ "" };
		boost::urls::pct_string_view(m[1]).decode({}, boost::urls::string_token::append_to(var2_msg));
		BOOST_CHECK_EQUAL("two", var2_msg);
	}

	{
		Routing::matches m;
		std::string_view* matches_it = m.matches();
		std::string_view* ids_it = m.ids();

		auto none = boost::urls::parse_path("/optional");
		BOOST_TEST_REQUIRE(!none.has_error());
		auto p = test_routes.find_impl(*none, matches_it, ids_it);
		BOOST_CHECK_NE(nullptr, p);
	}

	{
		Routing::matches m;
		std::string_view* matches_it = m.matches();
		std::string_view* ids_it = m.ids();

		auto star0 = boost::urls::parse_path("/star");
		BOOST_TEST_REQUIRE(!star0.has_error());
		auto p0 = test_routes.find_impl(*star0, matches_it, ids_it);
		BOOST_CHECK_NE(nullptr, p0);

		std::string star0_msg{ "" };
		boost::urls::pct_string_view(m[0]).decode({}, boost::urls::string_token::append_to(star0_msg));
		BOOST_CHECK_EQUAL("", star0_msg);

		auto star1 = boost::urls::parse_path("/star/this");
		BOOST_TEST_REQUIRE(!star1.has_error());
		auto p1 = test_routes.find_impl(*star1, matches_it, ids_it);
		BOOST_CHECK_NE(nullptr, p1);

		std::string star1_msg{ "" };
		boost::urls::pct_string_view(m[1]).decode({}, boost::urls::string_token::append_to(star1_msg));
		BOOST_CHECK_EQUAL("this", star1_msg);

		auto star2 = boost::urls::parse_path("/star/this/is");
		BOOST_TEST_REQUIRE(!star2.has_error());
		auto p2 = test_routes.find_impl(*star2, matches_it, ids_it);
		BOOST_CHECK_NE(nullptr, p2);

		std::string star2_msg{ "" };
		boost::urls::pct_string_view(m[2]).decode({}, boost::urls::string_token::append_to(star2_msg));
		BOOST_CHECK_EQUAL("this/is", star2_msg);
	}

	{
		Routing::matches m;
		std::string_view* matches_it = m.matches();
		std::string_view* ids_it = m.ids();

		auto plus1 = boost::urls::parse_path("/plus");
		BOOST_TEST_REQUIRE(!plus1.has_error());
		auto p1 = test_routes.find_impl(*plus1, matches_it, ids_it);
		BOOST_CHECK_EQUAL(nullptr, p1);

		auto plus2 = boost::urls::parse_path("/plus/this");
		BOOST_TEST_REQUIRE(!plus2.has_error());
		auto p2 = test_routes.find_impl(*plus2, matches_it, ids_it);
		BOOST_CHECK_NE(nullptr, p2);

		auto plus3 = boost::urls::parse_path("/plus/this/is");
		BOOST_TEST_REQUIRE(!plus3.has_error());
		auto p3 = test_routes.find_impl(*plus3, matches_it, ids_it);
		BOOST_CHECK_NE(nullptr, p3);

		auto plus7 = boost::urls::parse_path("/plus/this/is/a/test/path/index.html");
		BOOST_TEST_REQUIRE(!plus7.has_error());
		auto p4 = test_routes.find_impl(*plus7, matches_it, ids_it);
		BOOST_CHECK_NE(nullptr, p4);
	}
}

BOOST_AUTO_TEST_SUITE_END()
