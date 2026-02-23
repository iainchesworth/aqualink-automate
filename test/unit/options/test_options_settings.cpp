#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "options/options_settings.h"

using namespace AqualinkAutomate::Options;

namespace
{
	struct TestSettingsA
	{
		static std::string AreaName() { return "test_area_a"; }
		int value{ 0 };
	};

	struct TestSettingsB
	{
		static std::string AreaName() { return "test_area_b"; }
		std::string name;
	};
}

BOOST_AUTO_TEST_SUITE(OptionsSettings_TestSuite)

// =============================================================================
// Default state
// =============================================================================

BOOST_AUTO_TEST_CASE(TestDefault_HasReturnsFalse)
{
	Settings settings;
	BOOST_CHECK(!settings.Has("test_area_a"));
}

BOOST_AUTO_TEST_CASE(TestDefault_GetAreasEmpty)
{
	Settings settings;
	auto areas = settings.GetAreas();
	BOOST_CHECK(areas.empty());
}

BOOST_AUTO_TEST_CASE(TestDefault_GetReturnsUnexpected)
{
	Settings settings;
	auto result = settings.Get<TestSettingsA>();
	BOOST_CHECK(!result.has_value());
}

// =============================================================================
// Set and Get
// =============================================================================

BOOST_AUTO_TEST_CASE(TestSet_ThenHas_ReturnsTrue)
{
	Settings settings;
	settings.Set("test_area_a", TestSettingsA{42});
	BOOST_CHECK(settings.Has("test_area_a"));
}

BOOST_AUTO_TEST_CASE(TestSet_ThenGet_ReturnsValue)
{
	Settings settings;
	settings.Set("test_area_a", TestSettingsA{42});
	auto result = settings.Get<TestSettingsA>();
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result->get().value, 42);
}

BOOST_AUTO_TEST_CASE(TestSet_MultipleAreas)
{
	Settings settings;
	settings.Set("test_area_a", TestSettingsA{10});
	settings.Set("test_area_b", TestSettingsB{"hello"});

	auto areas = settings.GetAreas();
	BOOST_CHECK_EQUAL(areas.size(), 2);

	auto resultA = settings.Get<TestSettingsA>();
	BOOST_REQUIRE(resultA.has_value());
	BOOST_CHECK_EQUAL(resultA->get().value, 10);

	auto resultB = settings.Get<TestSettingsB>();
	BOOST_REQUIRE(resultB.has_value());
	BOOST_CHECK_EQUAL(resultB->get().name, "hello");
}

BOOST_AUTO_TEST_CASE(TestSet_Overwrite)
{
	Settings settings;
	settings.Set("test_area_a", TestSettingsA{10});
	settings.Set("test_area_a", TestSettingsA{99});

	auto result = settings.Get<TestSettingsA>();
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result->get().value, 99);
}

// =============================================================================
// Get with wrong type
// =============================================================================

BOOST_AUTO_TEST_CASE(TestGet_WrongType_ReturnsUnexpected)
{
	Settings settings;
	settings.Set("test_area_a", TestSettingsA{42});

	// Storing TestSettingsA under "test_area_a" key, but trying to get as TestSettingsB
	// which has a different AreaName, so it will look for "test_area_b"
	auto result = settings.Get<TestSettingsB>();
	BOOST_CHECK(!result.has_value());
}

// =============================================================================
// Clear
// =============================================================================

BOOST_AUTO_TEST_CASE(TestClear_RemovesAllAreas)
{
	Settings settings;
	settings.Set("test_area_a", TestSettingsA{10});
	settings.Set("test_area_b", TestSettingsB{"hello"});

	settings.Clear();

	BOOST_CHECK(!settings.Has("test_area_a"));
	BOOST_CHECK(!settings.Has("test_area_b"));
	BOOST_CHECK(settings.GetAreas().empty());
}

BOOST_AUTO_TEST_SUITE_END()
