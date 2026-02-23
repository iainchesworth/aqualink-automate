#include <boost/test/unit_test.hpp>

#include <boost/program_options.hpp>

#include "kernel/pool_configurations.h"
#include "options/options_equipment_options.h"

using namespace AqualinkAutomate;
namespace po = boost::program_options;

namespace
{
	/// Helper to create a variables_map from simulated command line arguments using
	/// the Equipment OptionsProcessor's options description.
	po::variables_map ParseEquipmentOptions(Options::Equipment::OptionsProcessor& processor, const std::vector<const char*>& args)
	{
		po::options_description desc;
		desc.add(processor.Options());

		po::variables_map vm;
		po::store(po::parse_command_line(static_cast<int>(args.size()), args.data(), desc), vm);
		po::notify(vm);

		return vm;
	}
}

BOOST_AUTO_TEST_SUITE(TestSuite_EquipmentOptions)

//-----------------------------------------------------------------------------
// DEFAULT VALUES
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_EquipmentOptions_DefaultSettings)
{
	Options::Equipment::EquipmentSettings settings;

	BOOST_CHECK(settings.pool_configuration == Kernel::PoolConfigurations::Unknown);
	BOOST_CHECK(!settings.pool_configuration_is_user_specified);
}

BOOST_AUTO_TEST_CASE(Test_EquipmentOptions_ProcessDefaults)
{
	Options::Equipment::OptionsProcessor processor;
	auto vm = ParseEquipmentOptions(processor, { "program" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	const auto& settings = result.value();

	// "auto" default should leave as Unknown, not user-specified
	BOOST_CHECK(settings.pool_configuration == Kernel::PoolConfigurations::Unknown);
	BOOST_CHECK(!settings.pool_configuration_is_user_specified);
}

//-----------------------------------------------------------------------------
// POOL CONFIGURATION MAPPINGS
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_EquipmentOptions_PoolOnly)
{
	Options::Equipment::OptionsProcessor processor;
	auto vm = ParseEquipmentOptions(processor, { "program", "--pool-configuration=pool-only" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK(result.value().pool_configuration == Kernel::PoolConfigurations::SingleBody);
	BOOST_CHECK(result.value().pool_configuration_is_user_specified);
}

BOOST_AUTO_TEST_CASE(Test_EquipmentOptions_SpaOnly)
{
	Options::Equipment::OptionsProcessor processor;
	auto vm = ParseEquipmentOptions(processor, { "program", "--pool-configuration=spa-only" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK(result.value().pool_configuration == Kernel::PoolConfigurations::SingleBody);
	BOOST_CHECK(result.value().pool_configuration_is_user_specified);
}

BOOST_AUTO_TEST_CASE(Test_EquipmentOptions_Combo)
{
	Options::Equipment::OptionsProcessor processor;
	auto vm = ParseEquipmentOptions(processor, { "program", "--pool-configuration=combo" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK(result.value().pool_configuration == Kernel::PoolConfigurations::DualBody_SharedEquipment);
	BOOST_CHECK(result.value().pool_configuration_is_user_specified);
}

BOOST_AUTO_TEST_CASE(Test_EquipmentOptions_Dual)
{
	Options::Equipment::OptionsProcessor processor;
	auto vm = ParseEquipmentOptions(processor, { "program", "--pool-configuration=dual" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK(result.value().pool_configuration == Kernel::PoolConfigurations::DualBody_DualEquipment);
	BOOST_CHECK(result.value().pool_configuration_is_user_specified);
}

BOOST_AUTO_TEST_CASE(Test_EquipmentOptions_Auto)
{
	Options::Equipment::OptionsProcessor processor;
	auto vm = ParseEquipmentOptions(processor, { "program", "--pool-configuration=auto" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK(result.value().pool_configuration == Kernel::PoolConfigurations::Unknown);
	BOOST_CHECK(!result.value().pool_configuration_is_user_specified);
}

//-----------------------------------------------------------------------------
// INVALID VALUES
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_EquipmentOptions_UnknownValue_TreatedAsAuto)
{
	// Unrecognized values should fall through to "auto" (Unknown, not user-specified)
	Options::Equipment::OptionsProcessor processor;
	auto vm = ParseEquipmentOptions(processor, { "program", "--pool-configuration=foobar" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK(result.value().pool_configuration == Kernel::PoolConfigurations::Unknown);
	BOOST_CHECK(!result.value().pool_configuration_is_user_specified);
}

//-----------------------------------------------------------------------------
// VALIDATION
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_EquipmentOptions_ValidateNoThrow)
{
	Options::Equipment::OptionsProcessor processor;
	auto vm = ParseEquipmentOptions(processor, { "program" });

	BOOST_CHECK_NO_THROW(processor.Validate(vm));
}

BOOST_AUTO_TEST_SUITE_END()
