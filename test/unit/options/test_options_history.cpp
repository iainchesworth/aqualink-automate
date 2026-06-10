#include <boost/test/unit_test.hpp>

#include <boost/program_options.hpp>

#include "exceptions/exception_optionparsingfailed.h"
#include "options/options_history_options.h"

using namespace AqualinkAutomate;
namespace po = boost::program_options;

namespace
{
	po::variables_map ParseHistoryOptions(Options::History::OptionsProcessor& processor, const std::vector<const char*>& args)
	{
		po::options_description desc;
		desc.add(processor.Options());

		po::variables_map vm;
		po::store(po::parse_command_line(static_cast<int>(args.size()), args.data(), desc), vm);
		po::notify(vm);
		return vm;
	}
}

BOOST_AUTO_TEST_SUITE(TestSuite_HistoryOptions)

BOOST_AUTO_TEST_CASE(Test_HistoryOptions_DefaultsDisabled)
{
	Options::History::OptionsProcessor processor;
	auto vm = ParseHistoryOptions(processor, { "program" });

	BOOST_CHECK_NO_THROW(processor.Validate(vm));
	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	const auto& settings = result.value();
	BOOST_CHECK(settings.db_path.empty()); // empty == history disabled
	BOOST_CHECK_EQUAL(settings.retention_days, 90u);
	BOOST_CHECK_EQUAL(settings.flush_seconds, 10u);
}

BOOST_AUTO_TEST_CASE(Test_HistoryOptions_CustomValues)
{
	Options::History::OptionsProcessor processor;
	auto vm = ParseHistoryOptions(processor, { "program",
		"--history-db", "/var/lib/aqualink/history.db",
		"--history-retention-days", "30",
		"--history-flush-seconds", "5" });

	BOOST_CHECK_NO_THROW(processor.Validate(vm));
	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	const auto& settings = result.value();
	BOOST_CHECK_EQUAL(settings.db_path, "/var/lib/aqualink/history.db");
	BOOST_CHECK_EQUAL(settings.retention_days, 30u);
	BOOST_CHECK_EQUAL(settings.flush_seconds, 5u);
}

BOOST_AUTO_TEST_CASE(Test_HistoryOptions_ZeroRetentionRejected)
{
	Options::History::OptionsProcessor processor;
	auto vm = ParseHistoryOptions(processor, { "program", "--history-retention-days", "0" });
	BOOST_CHECK_THROW(processor.Validate(vm), Exceptions::OptionParsingFailed);
}

BOOST_AUTO_TEST_CASE(Test_HistoryOptions_ZeroFlushRejected)
{
	Options::History::OptionsProcessor processor;
	auto vm = ParseHistoryOptions(processor, { "program", "--history-flush-seconds", "0" });
	BOOST_CHECK_THROW(processor.Validate(vm), Exceptions::OptionParsingFailed);
}

BOOST_AUTO_TEST_SUITE_END()
