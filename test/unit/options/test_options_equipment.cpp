#include <boost/test/unit_test.hpp>

#include <boost/program_options.hpp>

#include "kernel/body_of_water_ids.h"
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
	BOOST_CHECK(settings.single_body_kind == Kernel::BodyOfWaterIds::Pool);
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
	// pool-only -> the single body is a Pool body.
	BOOST_CHECK(result.value().single_body_kind == Kernel::BodyOfWaterIds::Pool);
}

BOOST_AUTO_TEST_CASE(Test_EquipmentOptions_SpaOnly)
{
	Options::Equipment::OptionsProcessor processor;
	auto vm = ParseEquipmentOptions(processor, { "program", "--pool-configuration=spa-only" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());

	BOOST_CHECK(result.value().pool_configuration == Kernel::PoolConfigurations::SingleBody);
	BOOST_CHECK(result.value().pool_configuration_is_user_specified);
	// spa-only -> same SingleBody config, but the single body is a Spa body.
	BOOST_CHECK(result.value().single_body_kind == Kernel::BodyOfWaterIds::Spa);
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
// EQUIPMENT CACHE FILE
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_EquipmentOptions_CacheFileDefaultEmpty)
{
	Options::Equipment::OptionsProcessor processor;
	auto vm = ParseEquipmentOptions(processor, { "program" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK(result.value().equipment_cache_file.empty());
}

BOOST_AUTO_TEST_CASE(Test_EquipmentOptions_CacheFileCustom)
{
	Options::Equipment::OptionsProcessor processor;
	auto vm = ParseEquipmentOptions(processor, { "program", "--equipment-cache-file", "/var/lib/aqualink/equip.json" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result.value().equipment_cache_file, "/var/lib/aqualink/equip.json");
}

//-----------------------------------------------------------------------------
// TEMPERATURE STALENESS THRESHOLD
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_EquipmentOptions_StalenessThresholdDefault)
{
	Options::Equipment::OptionsProcessor processor;
	auto vm = ParseEquipmentOptions(processor, { "program" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result.value().temperature_staleness_threshold_seconds, 600u);
}

BOOST_AUTO_TEST_CASE(Test_EquipmentOptions_StalenessThresholdCustom)
{
	Options::Equipment::OptionsProcessor processor;
	auto vm = ParseEquipmentOptions(processor, { "program", "--temperature-staleness-threshold", "1800" });

	auto result = processor.Process(vm);
	BOOST_REQUIRE(result.has_value());
	BOOST_CHECK_EQUAL(result.value().temperature_staleness_threshold_seconds, 1800u);
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
