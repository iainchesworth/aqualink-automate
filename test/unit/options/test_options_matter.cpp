#include <boost/test/unit_test.hpp>

#include <boost/program_options.hpp>

#include "options/options_config_file.h"
#include "options/options_matter_options.h"
#include "options/options_registry.h"
#include "options/options_app_options.h"

using namespace AqualinkAutomate;
namespace po = boost::program_options;

namespace
{
	/// Helper to create a variables_map from simulated command line arguments using
	/// the Matter OptionsProcessor's options description.
	po::variables_map ParseMatterOptions(Options::Matter::OptionsProcessor& processor, const std::vector<const char*>& args)
	{
		po::options_description desc;
		desc.add(processor.Options());

		po::variables_map vm;
		po::store(po::parse_command_line(static_cast<int>(args.size()), args.data(), desc), vm);
		po::notify(vm);

		return vm;
	}
}

BOOST_AUTO_TEST_SUITE(TestSuite_MatterOptions)

//-----------------------------------------------------------------------------
// OPTIONS DESCRIPTION
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_MatterOptions_DescribesAllOptions)
{
	Options::Matter::OptionsProcessor processor;
	auto desc = processor.Options();

	// All five Matter options must be registered (enable, storage-path,
	// status-port, passcode, discriminator).
	BOOST_CHECK_EQUAL(desc.options().size(), 5u);
}

//-----------------------------------------------------------------------------
// DEFAULT VALUES
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_MatterOptions_DefaultSettings)
{
	Options::Matter::OptionsProcessor processor;
	auto vm = ParseMatterOptions(processor, { "program" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	const auto& settings = result.value();

	// Matter is opt-OUT: default on.
	BOOST_CHECK(settings.enabled);
	BOOST_CHECK_EQUAL(settings.storage_path, "/data/matter");
	BOOST_CHECK_EQUAL(settings.status_port, 8099);
	BOOST_CHECK_EQUAL(settings.passcode, 0u);
	BOOST_CHECK_EQUAL(settings.discriminator, 0u);
}

//-----------------------------------------------------------------------------
// VALIDATE (no cross-option dependencies — always succeeds)
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_MatterOptions_Validate_NeverThrows)
{
	Options::Matter::OptionsProcessor processor;
	auto vm = ParseMatterOptions(processor, { "program",
		"--matter=false",
		"--matter-passcode=12345678",
		"--matter-discriminator=4095" });

	BOOST_CHECK_NO_THROW(processor.Validate(vm));
}

//-----------------------------------------------------------------------------
// OPT-OUT FLAG
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_MatterOptions_DisableViaExplicitFalse)
{
	Options::Matter::OptionsProcessor processor;
	auto vm = ParseMatterOptions(processor, { "program", "--matter=false" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK(!result.value().enabled);
}

BOOST_AUTO_TEST_CASE(Test_MatterOptions_EnableViaImplicitFlag)
{
	// `--matter` with no value uses the implicit value (true).
	Options::Matter::OptionsProcessor processor;
	auto vm = ParseMatterOptions(processor, { "program", "--matter" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK(result.value().enabled);
}

//-----------------------------------------------------------------------------
// INDIVIDUAL OVERRIDES (each exercises a distinct Process() branch)
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_MatterOptions_CustomStoragePath)
{
	Options::Matter::OptionsProcessor processor;
	auto vm = ParseMatterOptions(processor, { "program", "--matter-storage-path=/srv/matter-state" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK_EQUAL(result.value().storage_path, "/srv/matter-state");
}

BOOST_AUTO_TEST_CASE(Test_MatterOptions_CustomStatusPort)
{
	Options::Matter::OptionsProcessor processor;
	auto vm = ParseMatterOptions(processor, { "program", "--matter-status-port=9000" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK_EQUAL(result.value().status_port, 9000);
}

BOOST_AUTO_TEST_CASE(Test_MatterOptions_CustomPasscode)
{
	Options::Matter::OptionsProcessor processor;
	auto vm = ParseMatterOptions(processor, { "program", "--matter-passcode=20202021" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK_EQUAL(result.value().passcode, 20202021u);
}

BOOST_AUTO_TEST_CASE(Test_MatterOptions_CustomDiscriminator)
{
	Options::Matter::OptionsProcessor processor;
	auto vm = ParseMatterOptions(processor, { "program", "--matter-discriminator=3840" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK_EQUAL(result.value().discriminator, 3840);
}

//-----------------------------------------------------------------------------
// FULL PIPELINE (Add + Parse + Notify + Validate + Process + Finalise)
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_MatterOptions_FullPipeline_OptOut)
{
	const char* argv[] = { "program", "--matter=false", "--matter-status-port=9100" };
	int argc = 3;

	auto processed_options = Options::Initialise()
		| Options::Add(Options::Matter::OptionsProcessor{})
		| Options::Parse(argc, const_cast<char**>(argv))
		| Options::Notify()
		| Options::Validate()
		| Options::Process(Options::Matter::OptionsProcessor{})
		| Options::Finalise();

	BOOST_REQUIRE(processed_options.has_value());

	auto matter_settings_result = processed_options.value().Get<Options::Matter::MatterSettings>();
	BOOST_REQUIRE(matter_settings_result.has_value());

	const auto& settings = matter_settings_result.value().get();

	BOOST_CHECK(!settings.enabled);
	BOOST_CHECK_EQUAL(settings.status_port, 9100);
}

BOOST_AUTO_TEST_CASE(Test_MatterOptions_FullPipeline_Defaults)
{
	const char* argv[] = { "program" };
	int argc = 1;

	auto processed_options = Options::Initialise()
		| Options::Add(Options::Matter::OptionsProcessor{})
		| Options::Parse(argc, const_cast<char**>(argv))
		| Options::Notify()
		| Options::Validate()
		| Options::Process(Options::Matter::OptionsProcessor{})
		| Options::Finalise();

	BOOST_REQUIRE(processed_options.has_value());

	auto matter_settings_result = processed_options.value().Get<Options::Matter::MatterSettings>();
	BOOST_REQUIRE(matter_settings_result.has_value());

	const auto& settings = matter_settings_result.value().get();

	BOOST_CHECK(settings.enabled);
	BOOST_CHECK_EQUAL(settings.storage_path, "/data/matter");
	BOOST_CHECK_EQUAL(settings.status_port, 8099);
}

BOOST_AUTO_TEST_SUITE_END()
