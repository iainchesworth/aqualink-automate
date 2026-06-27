#include <boost/test/unit_test.hpp>

#include <boost/program_options.hpp>

#include "options/options_config_file.h"
#include "options/options_web_options.h"
#include "options/options_registry.h"
#include "options/options_app_options.h"
#include "exceptions/exception_options_conflictingoptions.h"
#include "exceptions/exception_options_missingdependency.h"

using namespace AqualinkAutomate;
namespace po = boost::program_options;

namespace
{
	/// Helper to create a variables_map from simulated command line arguments using
	/// the Web OptionsProcessor's options description.
	po::variables_map ParseWebOptions(Options::Web::OptionsProcessor& processor, const std::vector<const char*>& args)
	{
		po::options_description desc;
		desc.add(processor.Options());

		po::variables_map vm;
		po::store(po::parse_command_line(static_cast<int>(args.size()), args.data(), desc), vm);
		po::notify(vm);

		return vm;
	}
}

BOOST_AUTO_TEST_SUITE(TestSuite_WebOptions)

//-----------------------------------------------------------------------------
// DEFAULT VALUES
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_WebOptions_DefaultSettings)
{
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	const auto& settings = result.value();

	BOOST_CHECK(settings.http_server_is_enabled);
	BOOST_CHECK(settings.https_server_is_enabled);
	BOOST_CHECK(!settings.http_content_is_disabled);
	BOOST_CHECK_EQUAL(settings.bind_address, "127.0.0.1");
	BOOST_CHECK_EQUAL(settings.http_port, 80);
	BOOST_CHECK_EQUAL(settings.https_port, 443);
}

//-----------------------------------------------------------------------------
// DISABLE HTTPS
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_WebOptions_DisableHttps_DisablesHttpsServer)
{
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program", "--disable-https" });

	BOOST_CHECK_NO_THROW(processor.Validate(vm));

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	const auto& settings = result.value();

	BOOST_CHECK(!settings.https_server_is_enabled);
	BOOST_CHECK(settings.http_server_is_enabled);
}

BOOST_AUTO_TEST_CASE(Test_WebOptions_DisableHttps_HttpStillEnabled)
{
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program", "--disable-https" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK(result.value().http_server_is_enabled);
}

//-----------------------------------------------------------------------------
// DISABLE HTTP
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_WebOptions_DisableHttp_DisablesHttpServer)
{
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program", "--disable-http" });

	BOOST_CHECK_NO_THROW(processor.Validate(vm));

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	const auto& settings = result.value();

	BOOST_CHECK(!settings.http_server_is_enabled);
	BOOST_CHECK(settings.https_server_is_enabled);
}

//-----------------------------------------------------------------------------
// DISABLE BOTH
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_WebOptions_DisableBoth)
{
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program", "--disable-https", "--disable-http" });

	BOOST_CHECK_NO_THROW(processor.Validate(vm));

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	const auto& settings = result.value();

	BOOST_CHECK(!settings.http_server_is_enabled);
	BOOST_CHECK(!settings.https_server_is_enabled);
}

//-----------------------------------------------------------------------------
// CONFLICTING OPTIONS
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_WebOptions_DisableHttps_ConflictsWithExplicitCert)
{
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program", "--disable-https", "--cert=my_cert.pem", "--cert-key=my_key.pem" });

	BOOST_CHECK_THROW(processor.Validate(vm), Exceptions::Options_ConflictingOptions);
}

BOOST_AUTO_TEST_CASE(Test_WebOptions_DisableHttps_ConflictsWithExplicitHttpsPort)
{
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program", "--disable-https", "--https-port=8443" });

	BOOST_CHECK_THROW(processor.Validate(vm), Exceptions::Options_ConflictingOptions);
}

BOOST_AUTO_TEST_CASE(Test_WebOptions_DisableHttp_ConflictsWithExplicitHttpPort)
{
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program", "--disable-http", "--http-port=8080" });

	BOOST_CHECK_THROW(processor.Validate(vm), Exceptions::Options_ConflictingOptions);
}

BOOST_AUTO_TEST_CASE(Test_WebOptions_DisableHttps_NoConflictWithDefaultedCert)
{
	// When only --disable-https is passed, the cert/cert-key options
	// still have their default values. This should NOT be treated as
	// a conflict, since the user did not explicitly provide them.
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program", "--disable-https" });

	BOOST_CHECK_NO_THROW(processor.Validate(vm));
}

//-----------------------------------------------------------------------------
// OPTION DEPENDENCIES
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_WebOptions_CertRequiresCertKey)
{
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program", "--cert=my_cert.pem" });

	// --cert was explicitly provided but --cert-key was not (only defaulted).
	// The dependency helper should flag this.
	BOOST_CHECK_THROW(processor.Validate(vm), Exceptions::Options_MissingDependency);
}

BOOST_AUTO_TEST_CASE(Test_WebOptions_CertKeyRequiresCert)
{
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program", "--cert-key=my_key.pem" });

	BOOST_CHECK_THROW(processor.Validate(vm), Exceptions::Options_MissingDependency);
}

BOOST_AUTO_TEST_CASE(Test_WebOptions_CertAndCertKeyTogether)
{
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program", "--cert=my_cert.pem", "--cert-key=my_key.pem" });

	BOOST_CHECK_NO_THROW(processor.Validate(vm));

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK_EQUAL(result.value().ssl_certificate.certificate.string(), "my_cert.pem");
	BOOST_CHECK_EQUAL(result.value().ssl_certificate.private_key.string(), "my_key.pem");
}

//-----------------------------------------------------------------------------
// PORT CONFIGURATION
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_WebOptions_CustomPorts)
{
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program", "--http-port=8080", "--https-port=8443" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK_EQUAL(result.value().http_port, 8080);
	BOOST_CHECK_EQUAL(result.value().https_port, 8443);
}

//-----------------------------------------------------------------------------
// BIND ADDRESS
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_WebOptions_CustomBindAddress)
{
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program", "--address=127.0.0.1" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK_EQUAL(result.value().bind_address, "127.0.0.1");
}

//-----------------------------------------------------------------------------
// API AUTH TOKEN (opt-in, default UNSET)
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_WebOptions_ApiAuthToken_DefaultUnset)
{
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	// Default behaviour: no token => no auth, exactly as before.
	BOOST_CHECK(!result.value().ApiAuthToken.has_value());
}

BOOST_AUTO_TEST_CASE(Test_WebOptions_ApiAuthToken_SetWhenProvided)
{
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program", "--api-auth-token=s3cr3t-token" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_REQUIRE(result.value().ApiAuthToken.has_value());
	BOOST_CHECK_EQUAL(result.value().ApiAuthToken.value(), "s3cr3t-token");
}

//-----------------------------------------------------------------------------
// ORIGIN ALLOW-LIST + CSRF HEADER + INSECURE ACK (previously-unreachable knobs)
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_WebOptions_CrossSiteKnobs_DefaultOff)
{
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK(result.value().ApiAllowedOrigins.empty());
	BOOST_CHECK(!result.value().ApiRequireCsrfHeader);
	BOOST_CHECK(!result.value().InsecureNoAuthAck);
}

BOOST_AUTO_TEST_CASE(Test_WebOptions_AllowedOrigin_Repeatable)
{
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program",
		"--api-allowed-origin=https://pool.example",
		"--api-allowed-origin=https://app.example" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	const auto& origins = result.value().ApiAllowedOrigins;
	BOOST_REQUIRE_EQUAL(origins.size(), 2u);
	BOOST_CHECK_EQUAL(origins[0], "https://pool.example");
	BOOST_CHECK_EQUAL(origins[1], "https://app.example");
}

BOOST_AUTO_TEST_CASE(Test_WebOptions_RequireCsrfHeader_SetWhenProvided)
{
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program", "--api-require-csrf-header" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK(result.value().ApiRequireCsrfHeader);
}

BOOST_AUTO_TEST_CASE(Test_WebOptions_InsecureNoAuthAck_SetWhenProvided)
{
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program", "--insecure-no-auth" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK(result.value().InsecureNoAuthAck);
}

//-----------------------------------------------------------------------------
// CONTENT DISABLE
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_WebOptions_DisableContent)
{
	Options::Web::OptionsProcessor processor;
	auto vm = ParseWebOptions(processor, { "program", "--disable-content" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK(result.value().http_content_is_disabled);
}

//-----------------------------------------------------------------------------
// FULL PIPELINE (tests the same flow as main: Add + Parse + Validate + Process)
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_WebOptions_FullPipeline_DisableHttps)
{
	// This test simulates the full options pipeline as used in main:
	// - Add() uses one OptionsProcessor instance to register options
	// - Process() uses a different OptionsProcessor instance to extract settings
	// This verifies that the pipeline correctly handles --disable-https.

	const char* argv[] = { "program", "--disable-https" };
	int argc = 2;

	auto processed_options = Options::Initialise()
		| Options::Add(Options::Web::OptionsProcessor{})
		| Options::Parse(argc, const_cast<char**>(argv))
		| Options::Notify()
		| Options::Validate()
		| Options::Process(Options::Web::OptionsProcessor{})
		| Options::Finalise();

	BOOST_REQUIRE(processed_options.has_value());

	auto web_settings_result = processed_options.value().Get<Options::Web::WebSettings>();
	BOOST_REQUIRE(web_settings_result.has_value());

	const auto& settings = web_settings_result.value().get();

	BOOST_CHECK(!settings.https_server_is_enabled);
	BOOST_CHECK(settings.http_server_is_enabled);
}

BOOST_AUTO_TEST_CASE(Test_WebOptions_FullPipeline_DefaultsEnableHttps)
{
	const char* argv[] = { "program" };
	int argc = 1;

	auto processed_options = Options::Initialise()
		| Options::Add(Options::Web::OptionsProcessor{})
		| Options::Parse(argc, const_cast<char**>(argv))
		| Options::Notify()
		| Options::Validate()
		| Options::Process(Options::Web::OptionsProcessor{})
		| Options::Finalise();

	BOOST_REQUIRE(processed_options.has_value());

	auto web_settings_result = processed_options.value().Get<Options::Web::WebSettings>();
	BOOST_REQUIRE(web_settings_result.has_value());

	const auto& settings = web_settings_result.value().get();

	BOOST_CHECK(settings.https_server_is_enabled);
	BOOST_CHECK(settings.http_server_is_enabled);
}

BOOST_AUTO_TEST_CASE(Test_WebOptions_FullPipeline_DisableBoth)
{
	const char* argv[] = { "program", "--disable-https", "--disable-http" };
	int argc = 3;

	auto processed_options = Options::Initialise()
		| Options::Add(Options::Web::OptionsProcessor{})
		| Options::Parse(argc, const_cast<char**>(argv))
		| Options::Notify()
		| Options::Validate()
		| Options::Process(Options::Web::OptionsProcessor{})
		| Options::Finalise();

	BOOST_REQUIRE(processed_options.has_value());

	auto web_settings_result = processed_options.value().Get<Options::Web::WebSettings>();
	BOOST_REQUIRE(web_settings_result.has_value());

	const auto& settings = web_settings_result.value().get();

	BOOST_CHECK(!settings.https_server_is_enabled);
	BOOST_CHECK(!settings.http_server_is_enabled);
}

BOOST_AUTO_TEST_CASE(Test_WebOptions_FullPipeline_ConflictingOptionsFailsValidation)
{
	const char* argv[] = { "program", "--disable-https", "--cert=my_cert.pem", "--cert-key=my_key.pem" };
	int argc = 4;

	auto processed_options = Options::Initialise()
		| Options::Add(Options::Web::OptionsProcessor{})
		| Options::Parse(argc, const_cast<char**>(argv))
		| Options::Notify()
		| Options::Validate()
		| Options::Process(Options::Web::OptionsProcessor{})
		| Options::Finalise();

	// The Validate step catches the conflict and returns an error
	BOOST_CHECK(!processed_options.has_value());
}

BOOST_AUTO_TEST_SUITE_END()
