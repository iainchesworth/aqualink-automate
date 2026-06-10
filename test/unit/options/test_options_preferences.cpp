#include <boost/test/unit_test.hpp>

#include <boost/program_options.hpp>

#include "options/options_preferences_options.h"

using namespace AqualinkAutomate;
namespace po = boost::program_options;

namespace
{
	po::variables_map ParsePreferencesOptions(Options::Preferences::OptionsProcessor& processor, const std::vector<const char*>& args)
	{
		po::options_description desc;
		desc.add(processor.Options());
		po::variables_map vm;
		po::store(po::parse_command_line(static_cast<int>(args.size()), args.data(), desc), vm);
		po::notify(vm);
		return vm;
	}
}

BOOST_AUTO_TEST_SUITE(TestSuite_PreferencesOptions)

BOOST_AUTO_TEST_CASE(Test_PreferencesOptions_DefaultEmpty)
{
	Options::Preferences::OptionsProcessor processor;
	auto vm = ParsePreferencesOptions(processor, { "program" });
	BOOST_CHECK_NO_THROW(processor.Validate(vm));
	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result.value().preferences_file.empty());
}

BOOST_AUTO_TEST_CASE(Test_PreferencesOptions_CustomFile)
{
	Options::Preferences::OptionsProcessor processor;
	auto vm = ParsePreferencesOptions(processor, { "program", "--preferences-file", "/etc/aqualink/prefs.json" });
	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result.value().preferences_file, "/etc/aqualink/prefs.json");
}

BOOST_AUTO_TEST_SUITE_END()
