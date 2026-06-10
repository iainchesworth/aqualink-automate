#include <boost/test/unit_test.hpp>

#include <boost/program_options.hpp>

#include "options/options_scheduling_options.h"

using namespace AqualinkAutomate;
namespace po = boost::program_options;

namespace
{
	po::variables_map ParseSchedulingOptions(Options::Scheduling::OptionsProcessor& processor, const std::vector<const char*>& args)
	{
		po::options_description desc;
		desc.add(processor.Options());
		po::variables_map vm;
		po::store(po::parse_command_line(static_cast<int>(args.size()), args.data(), desc), vm);
		po::notify(vm);
		return vm;
	}
}

BOOST_AUTO_TEST_SUITE(TestSuite_SchedulingOptions)

BOOST_AUTO_TEST_CASE(Test_SchedulingOptions_DefaultDisabled)
{
	Options::Scheduling::OptionsProcessor processor;
	auto vm = ParseSchedulingOptions(processor, { "program" });
	BOOST_CHECK_NO_THROW(processor.Validate(vm));
	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result.value().schedules_file.empty());
}

BOOST_AUTO_TEST_CASE(Test_SchedulingOptions_CustomFile)
{
	Options::Scheduling::OptionsProcessor processor;
	auto vm = ParseSchedulingOptions(processor, { "program", "--schedules-file", "/etc/aqualink/schedules.json" });
	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result.value().schedules_file, "/etc/aqualink/schedules.json");
}

BOOST_AUTO_TEST_SUITE_END()
