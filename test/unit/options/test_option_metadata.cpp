#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <string>
#include <vector>

#include "options/options_option_type.h"

using namespace AqualinkAutomate;

BOOST_AUTO_TEST_SUITE(TestSuite_OptionMetadata)

//-----------------------------------------------------------------------------
// ACCESSORS
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_OptionMetadata_Accessors_WithShortName)
{
	auto option = Options::make_appoption("test-metadata-opt-with-short", "z", "Metadata test option with a short name");

	BOOST_CHECK_EQUAL(option->LongName(), "test-metadata-opt-with-short");
	BOOST_CHECK_EQUAL(option->ShortName(), "z");
	BOOST_CHECK_EQUAL(option->Description(), "Metadata test option with a short name");
}

BOOST_AUTO_TEST_CASE(Test_OptionMetadata_Accessors_WithoutShortName)
{
	auto option = Options::make_appoption("test-metadata-opt-no-short", "Metadata test option without a short name");

	BOOST_CHECK_EQUAL(option->LongName(), "test-metadata-opt-no-short");
	BOOST_CHECK_EQUAL(option->ShortName(), "");
	BOOST_CHECK_EQUAL(option->Description(), "Metadata test option without a short name");
}

//-----------------------------------------------------------------------------
// REGISTRY
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_OptionMetadata_Registry_ContainsConstructedOption)
{
	// Constructing the option snapshots its metadata into the core-level registry.
	auto option = Options::make_appoption("test-metadata-opt-with-short", "z", "Metadata test option with a short name");

	const auto registered = Options::AppOption::RegisteredOptions();

	const auto it = std::find_if(registered.begin(), registered.end(),
		[](const Options::OptionMetadata& meta)
		{
			return meta.long_name == "test-metadata-opt-with-short";
		});

	BOOST_REQUIRE(it != registered.end());
	BOOST_CHECK_EQUAL(it->short_name, "z");
	BOOST_CHECK_EQUAL(it->description, "Metadata test option with a short name");
}

BOOST_AUTO_TEST_CASE(Test_OptionMetadata_Registry_ContainsOptionWithoutShortName)
{
	auto option = Options::make_appoption("test-metadata-opt-no-short", "Metadata test option without a short name");

	const auto registered = Options::AppOption::RegisteredOptions();

	const auto it = std::find_if(registered.begin(), registered.end(),
		[](const Options::OptionMetadata& meta)
		{
			return meta.long_name == "test-metadata-opt-no-short";
		});

	BOOST_REQUIRE(it != registered.end());
	BOOST_CHECK_EQUAL(it->short_name, "");
	BOOST_CHECK_EQUAL(it->description, "Metadata test option without a short name");
}

BOOST_AUTO_TEST_SUITE_END()
