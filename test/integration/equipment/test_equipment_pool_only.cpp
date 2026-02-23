#include <boost/test/unit_test.hpp>

#include "kernel/auxillary_devices/auxillary_status.h"
#include "kernel/auxillary_devices/chlorinator_boost_mode.h"
#include "kernel/auxillary_devices/chlorinator_status.h"
#include "kernel/auxillary_devices/heater_status.h"
#include "kernel/auxillary_devices/pump_status.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/body_of_water_ids.h"
#include "kernel/pool_configurations.h"

#include "support/integration_test_pipeline.h"
#include "support/integration_test_scenarios.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Kernel;
using namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes;
using namespace AqualinkAutomate::Test;

//=============================================================================
// Pool-Only (SingleBody) Equipment Configuration Integration Tests
//=============================================================================

BOOST_FIXTURE_TEST_SUITE(TestSuite_Integration_Equipment_PoolOnly, IntegrationPipeline)

BOOST_AUTO_TEST_CASE(Test_PoolOnly_PoolConfigurationApplied)
{
	Scenarios::PopulatePoolOnly(*DataHub());

	BOOST_CHECK_EQUAL(static_cast<int>(DataHub()->PoolConfiguration), static_cast<int>(PoolConfigurations::SingleBody));
}

BOOST_AUTO_TEST_CASE(Test_PoolOnly_BodiesOfWaterCreated)
{
	Scenarios::PopulatePoolOnly(*DataHub());

	const auto& bodies = DataHub()->Bodies();
	BOOST_REQUIRE_EQUAL(bodies.size(), 1);
	BOOST_CHECK(bodies[0].Id() == BodyOfWaterIds::Pool);
}

BOOST_AUTO_TEST_CASE(Test_PoolOnly_TemperaturesSet)
{
	Scenarios::PopulatePoolOnly(*DataHub());

	auto air_temp = DataHub()->AirTemp();
	BOOST_REQUIRE(air_temp.has_value());
	BOOST_CHECK_CLOSE(air_temp->InFahrenheit().value(), 78.0, 1.0);

	auto pool_temp = DataHub()->PoolTemp();
	BOOST_REQUIRE(pool_temp.has_value());
	BOOST_CHECK_CLOSE(pool_temp->InFahrenheit().value(), 82.0, 1.0);

	auto pool_setpoint = DataHub()->PoolTempSetpoint();
	BOOST_REQUIRE(pool_setpoint.has_value());
	BOOST_CHECK_CLOSE(pool_setpoint->InFahrenheit().value(), 84.0, 1.0);

	auto freeze = DataHub()->FreezeProtectPoint();
	BOOST_REQUIRE(freeze.has_value());
	BOOST_CHECK_CLOSE(freeze->InFahrenheit().value(), 36.0, 1.0);
}

BOOST_AUTO_TEST_CASE(Test_PoolOnly_PumpCreated)
{
	Scenarios::PopulatePoolOnly(*DataHub());

	auto pumps = DataHub()->Pumps();
	BOOST_REQUIRE_EQUAL(pumps.size(), 1);

	auto label = pumps[0]->AuxillaryTraits.TryGet(LabelTrait{});
	BOOST_REQUIRE(label.has_value());
	BOOST_CHECK_EQUAL(label.value(), "Filter Pump");

	auto status = pumps[0]->AuxillaryTraits.TryGet(PumpStatusTrait{});
	BOOST_REQUIRE(status.has_value());
	BOOST_CHECK(status.value() == PumpStatuses::Running);
}

BOOST_AUTO_TEST_CASE(Test_PoolOnly_HeaterCreated)
{
	Scenarios::PopulatePoolOnly(*DataHub());

	auto heaters = DataHub()->Heaters();
	BOOST_REQUIRE_EQUAL(heaters.size(), 1);

	auto label = heaters[0]->AuxillaryTraits.TryGet(LabelTrait{});
	BOOST_REQUIRE(label.has_value());
	BOOST_CHECK_EQUAL(label.value(), "Pool Heat");

	auto status = heaters[0]->AuxillaryTraits.TryGet(HeaterStatusTrait{});
	BOOST_REQUIRE(status.has_value());
	BOOST_CHECK(status.value() == HeaterStatuses::Enabled);

	auto body = heaters[0]->AuxillaryTraits.TryGet(BodyOfWaterTrait{});
	BOOST_REQUIRE(body.has_value());
	BOOST_CHECK(body.value() == BodyOfWaterIds::Pool);
}

BOOST_AUTO_TEST_CASE(Test_PoolOnly_AuxillariesCreated)
{
	Scenarios::PopulatePoolOnly(*DataHub());

	auto auxillaries = DataHub()->Auxillaries();
	BOOST_REQUIRE_EQUAL(auxillaries.size(), 2);
}

BOOST_AUTO_TEST_CASE(Test_PoolOnly_ChlorinatorCreated)
{
	Scenarios::PopulatePoolOnly(*DataHub());

	auto chlorinators = DataHub()->Chlorinators();
	BOOST_REQUIRE_EQUAL(chlorinators.size(), 1);

	auto label = chlorinators[0]->AuxillaryTraits.TryGet(LabelTrait{});
	BOOST_REQUIRE(label.has_value());
	BOOST_CHECK_EQUAL(label.value(), "AquaPure");

	auto status = chlorinators[0]->AuxillaryTraits.TryGet(ChlorinatorStatusTrait{});
	BOOST_REQUIRE(status.has_value());
	BOOST_CHECK(status.value() == ChlorinatorStatuses::On);

	auto percentage = chlorinators[0]->AuxillaryTraits.TryGet(GeneratingPercentageTrait{});
	BOOST_REQUIRE(percentage.has_value());
	BOOST_CHECK_EQUAL(percentage.value(), 50);

	auto boost_mode = chlorinators[0]->AuxillaryTraits.TryGet(BoostModeTrait{});
	BOOST_REQUIRE(boost_mode.has_value());
	BOOST_CHECK(boost_mode.value() == ChlorinatorBoostModes::Off);

	auto health = chlorinators[0]->AuxillaryTraits.TryGet(ChlorinatorHealthTrait{});
	BOOST_REQUIRE(health.has_value());
	BOOST_CHECK(health.value() == ChlorinatorHealth::Ok);

	auto body = chlorinators[0]->AuxillaryTraits.TryGet(BodyOfWaterTrait{});
	BOOST_REQUIRE(body.has_value());
	BOOST_CHECK(body.value() == BodyOfWaterIds::Shared);
}

BOOST_AUTO_TEST_CASE(Test_PoolOnly_SaltLevelSet)
{
	Scenarios::PopulatePoolOnly(*DataHub());

	auto salt = DataHub()->SaltLevel();
	BOOST_CHECK_EQUAL(salt.value(), 3200);
}

BOOST_AUTO_TEST_CASE(Test_PoolOnly_TotalDeviceCount)
{
	Scenarios::PopulatePoolOnly(*DataHub());

	// 1 pump + 1 heater + 2 aux + 1 chlorinator = 5
	auto total = DataHub()->Pumps().size()
		+ DataHub()->Heaters().size()
		+ DataHub()->Auxillaries().size()
		+ DataHub()->Chlorinators().size();
	BOOST_CHECK_EQUAL(total, 5);
}

BOOST_AUTO_TEST_SUITE_END()
