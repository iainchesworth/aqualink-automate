#include <boost/test/unit_test.hpp>

#include <boost/program_options.hpp>

#include "exceptions/exception_optionparsingfailed.h"
#include "options/options_alerting_options.h"

using namespace AqualinkAutomate;
namespace po = boost::program_options;

namespace
{
	po::variables_map ParseAlertingOptions(Options::Alerting::OptionsProcessor& processor, const std::vector<const char*>& args)
	{
		po::options_description desc;
		desc.add(processor.Options());

		po::variables_map vm;
		po::store(po::parse_command_line(static_cast<int>(args.size()), args.data(), desc), vm);
		po::notify(vm);

		return vm;
	}
}

BOOST_AUTO_TEST_SUITE(TestSuite_AlertingOptions)

BOOST_AUTO_TEST_CASE(Test_AlertingOptions_Defaults)
{
	Options::Alerting::OptionsProcessor processor;
	auto vm = ParseAlertingOptions(processor, { "program" });

	BOOST_CHECK_NO_THROW(processor.Validate(vm));

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	const auto& settings = result.value();
	BOOST_CHECK_EQUAL(settings.salt_low_ppm, 2600u);
	BOOST_CHECK_EQUAL(settings.comms_timeout_seconds, 60u);
	BOOST_CHECK(settings.webhook_url.empty());
}

BOOST_AUTO_TEST_CASE(Test_AlertingOptions_CustomValues)
{
	Options::Alerting::OptionsProcessor processor;
	auto vm = ParseAlertingOptions(processor, { "program",
		"--alert-salt-low-ppm", "3000",
		"--alert-comms-timeout-seconds", "30",
		"--alert-webhook-url", "https://example.com/hook" });

	BOOST_CHECK_NO_THROW(processor.Validate(vm));

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	const auto& settings = result.value();
	BOOST_CHECK_EQUAL(settings.salt_low_ppm, 3000u);
	BOOST_CHECK_EQUAL(settings.comms_timeout_seconds, 30u);
	BOOST_CHECK_EQUAL(settings.webhook_url, "https://example.com/hook");
}

BOOST_AUTO_TEST_CASE(Test_AlertingOptions_SaltDisabledWithZero)
{
	Options::Alerting::OptionsProcessor processor;
	auto vm = ParseAlertingOptions(processor, { "program", "--alert-salt-low-ppm", "0" });

	BOOST_CHECK_NO_THROW(processor.Validate(vm));
	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result.value().salt_low_ppm, 0u);
}

BOOST_AUTO_TEST_CASE(Test_AlertingOptions_SaltOutOfRangeRejected)
{
	Options::Alerting::OptionsProcessor processor;
	auto vm = ParseAlertingOptions(processor, { "program", "--alert-salt-low-ppm", "9999" });

	BOOST_CHECK_THROW(processor.Validate(vm), Exceptions::OptionParsingFailed);
}

BOOST_AUTO_TEST_CASE(Test_AlertingOptions_ZeroCommsTimeoutRejected)
{
	Options::Alerting::OptionsProcessor processor;
	auto vm = ParseAlertingOptions(processor, { "program", "--alert-comms-timeout-seconds", "0" });

	BOOST_CHECK_THROW(processor.Validate(vm), Exceptions::OptionParsingFailed);
}

BOOST_AUTO_TEST_CASE(Test_AlertingOptions_InvalidWebhookUrlRejected)
{
	Options::Alerting::OptionsProcessor processor;
	auto vm = ParseAlertingOptions(processor, { "program", "--alert-webhook-url", "not-a-url" });

	BOOST_CHECK_THROW(processor.Validate(vm), Exceptions::OptionParsingFailed);
}

BOOST_AUTO_TEST_CASE(Test_AlertingOptions_FtpWebhookUrlRejected)
{
	Options::Alerting::OptionsProcessor processor;
	auto vm = ParseAlertingOptions(processor, { "program", "--alert-webhook-url", "ftp://example.com/x" });

	BOOST_CHECK_THROW(processor.Validate(vm), Exceptions::OptionParsingFailed);
}

BOOST_AUTO_TEST_SUITE_END()
