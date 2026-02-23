#include <boost/test/unit_test.hpp>

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
// Dual Body, Dual Equipment Configuration Integration Tests
//=============================================================================

BOOST_FIXTURE_TEST_SUITE(TestSuite_Integration_Equipment_DualBodyDual, IntegrationPipeline)

BOOST_AUTO_TEST_CASE(Test_DualBodyDual_PoolConfigurationApplied)
{
	Scenarios::PopulateDualBodyDual(*DataHub());

	BOOST_CHECK_EQUAL(
		static_cast<int>(DataHub()->PoolConfiguration),
		static_cast<int>(PoolConfigurations::DualBody_DualEquipment));
}

BOOST_AUTO_TEST_CASE(Test_DualBodyDual_TwoBodiesCreated)
{
	Scenarios::PopulateDualBodyDual(*DataHub());

	const auto& bodies = DataHub()->Bodies();
	BOOST_REQUIRE_EQUAL(bodies.size(), 2);
}

BOOST_AUTO_TEST_CASE(Test_DualBodyDual_AllTemperaturesSet)
{
	Scenarios::PopulateDualBodyDual(*DataHub());

	BOOST_REQUIRE(DataHub()->AirTemp().has_value());
	BOOST_CHECK_CLOSE(DataHub()->AirTemp()->InFahrenheit().value(), 74.0, 1.0);

	BOOST_REQUIRE(DataHub()->PoolTemp().has_value());
	BOOST_CHECK_CLOSE(DataHub()->PoolTemp()->InFahrenheit().value(), 79.0, 1.0);

	BOOST_REQUIRE(DataHub()->SpaTemp().has_value());
	BOOST_CHECK_CLOSE(DataHub()->SpaTemp()->InFahrenheit().value(), 99.0, 1.0);
}

BOOST_AUTO_TEST_CASE(Test_DualBodyDual_SeparatePumps)
{
	Scenarios::PopulateDualBodyDual(*DataHub());

	auto pumps = DataHub()->Pumps();
	BOOST_REQUIRE_EQUAL(pumps.size(), 2);

	bool found_pool_pump = false;
	bool found_spa_pump = false;

	for (const auto& pump : pumps)
	{
		auto label = pump->AuxillaryTraits.TryGet(LabelTrait{});
		BOOST_REQUIRE(label.has_value());

		auto body = pump->AuxillaryTraits.TryGet(BodyOfWaterTrait{});
		BOOST_REQUIRE(body.has_value());

		if (label.value() == "Pool Pump")
		{
			found_pool_pump = true;
			BOOST_CHECK(body.value() == BodyOfWaterIds::Pool);

			auto status = pump->AuxillaryTraits.TryGet(PumpStatusTrait{});
			BOOST_REQUIRE(status.has_value());
			BOOST_CHECK(status.value() == PumpStatuses::Running);
		}
		else if (label.value() == "Spa Pump")
		{
			found_spa_pump = true;
			BOOST_CHECK(body.value() == BodyOfWaterIds::Spa);

			auto status = pump->AuxillaryTraits.TryGet(PumpStatusTrait{});
			BOOST_REQUIRE(status.has_value());
			BOOST_CHECK(status.value() == PumpStatuses::Off);
		}
	}

	BOOST_CHECK(found_pool_pump);
	BOOST_CHECK(found_spa_pump);
}

BOOST_AUTO_TEST_CASE(Test_DualBodyDual_SeparateHeaters)
{
	Scenarios::PopulateDualBodyDual(*DataHub());

	auto heaters = DataHub()->Heaters();
	BOOST_REQUIRE_EQUAL(heaters.size(), 2);

	bool found_pool_heat = false;
	bool found_spa_heat = false;

	for (const auto& heater : heaters)
	{
		auto label = heater->AuxillaryTraits.TryGet(LabelTrait{});
		BOOST_REQUIRE(label.has_value());

		if (label.value() == "Pool Heat")
		{
			found_pool_heat = true;
			auto status = heater->AuxillaryTraits.TryGet(HeaterStatusTrait{});
			BOOST_REQUIRE(status.has_value());
			BOOST_CHECK(status.value() == HeaterStatuses::Heating);
		}
		else if (label.value() == "Spa Heat")
		{
			found_spa_heat = true;
			auto status = heater->AuxillaryTraits.TryGet(HeaterStatusTrait{});
			BOOST_REQUIRE(status.has_value());
			BOOST_CHECK(status.value() == HeaterStatuses::Off);
		}
	}

	BOOST_CHECK(found_pool_heat);
	BOOST_CHECK(found_spa_heat);
}

BOOST_AUTO_TEST_CASE(Test_DualBodyDual_ChlorinatorCreated)
{
	Scenarios::PopulateDualBodyDual(*DataHub());

	auto chlorinators = DataHub()->Chlorinators();
	BOOST_REQUIRE_EQUAL(chlorinators.size(), 1);

	auto percentage = chlorinators[0]->AuxillaryTraits.TryGet(GeneratingPercentageTrait{});
	BOOST_REQUIRE(percentage.has_value());
	BOOST_CHECK_EQUAL(percentage.value(), 70);
}

BOOST_AUTO_TEST_CASE(Test_DualBodyDual_TotalDeviceCount)
{
	Scenarios::PopulateDualBodyDual(*DataHub());

	// 2 pumps + 2 heaters + 2 aux + 1 chlorinator = 7
	auto total = DataHub()->Pumps().size()
		+ DataHub()->Heaters().size()
		+ DataHub()->Auxillaries().size()
		+ DataHub()->Chlorinators().size();
	BOOST_CHECK_EQUAL(total, 7);
}

BOOST_AUTO_TEST_SUITE_END()
