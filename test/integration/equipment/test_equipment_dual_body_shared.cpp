#include <boost/test/unit_test.hpp>

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
// Dual Body, Shared Equipment Configuration Integration Tests
//=============================================================================

BOOST_FIXTURE_TEST_SUITE(TestSuite_Integration_Equipment_DualBodyShared, IntegrationPipeline)

BOOST_AUTO_TEST_CASE(Test_DualBodyShared_PoolConfigurationApplied)
{
	Scenarios::PopulateDualBodyShared(*DataHub());

	BOOST_CHECK_EQUAL(
		static_cast<int>(DataHub()->PoolConfiguration),
		static_cast<int>(PoolConfigurations::DualBody_SharedEquipment));
}

BOOST_AUTO_TEST_CASE(Test_DualBodyShared_TwoBodiesCreated)
{
	Scenarios::PopulateDualBodyShared(*DataHub());

	const auto& bodies = DataHub()->Bodies();
	BOOST_REQUIRE_EQUAL(bodies.size(), 2);

	BOOST_CHECK(bodies[0].Id() == BodyOfWaterIds::Pool);
	BOOST_CHECK(bodies[1].Id() == BodyOfWaterIds::Spa);
}

BOOST_AUTO_TEST_CASE(Test_DualBodyShared_AllTemperaturesSet)
{
	Scenarios::PopulateDualBodyShared(*DataHub());

	BOOST_REQUIRE(DataHub()->AirTemp().has_value());
	BOOST_CHECK_CLOSE(DataHub()->AirTemp()->InFahrenheit().value(), 76.0, 1.0);

	BOOST_REQUIRE(DataHub()->PoolTemp().has_value());
	BOOST_CHECK_CLOSE(DataHub()->PoolTemp()->InFahrenheit().value(), 80.0, 1.0);

	BOOST_REQUIRE(DataHub()->SpaTemp().has_value());
	BOOST_CHECK_CLOSE(DataHub()->SpaTemp()->InFahrenheit().value(), 100.0, 1.0);

	BOOST_REQUIRE(DataHub()->PoolTempSetpoint().has_value());
	BOOST_CHECK_CLOSE(DataHub()->PoolTempSetpoint()->InFahrenheit().value(), 82.0, 1.0);

	BOOST_REQUIRE(DataHub()->SpaTempSetpoint().has_value());
	BOOST_CHECK_CLOSE(DataHub()->SpaTempSetpoint()->InFahrenheit().value(), 102.0, 1.0);
}

BOOST_AUTO_TEST_CASE(Test_DualBodyShared_SharedFilterPump)
{
	Scenarios::PopulateDualBodyShared(*DataHub());

	auto pumps = DataHub()->Pumps();
	BOOST_REQUIRE_EQUAL(pumps.size(), 1);

	auto label = pumps[0]->AuxillaryTraits.TryGet(LabelTrait{});
	BOOST_REQUIRE(label.has_value());
	BOOST_CHECK_EQUAL(label.value(), "Filter Pump");

	auto body = pumps[0]->AuxillaryTraits.TryGet(BodyOfWaterTrait{});
	BOOST_REQUIRE(body.has_value());
	BOOST_CHECK(body.value() == BodyOfWaterIds::Shared);
}

BOOST_AUTO_TEST_CASE(Test_DualBodyShared_TwoHeaters)
{
	Scenarios::PopulateDualBodyShared(*DataHub());

	auto heaters = DataHub()->Heaters();
	BOOST_REQUIRE_EQUAL(heaters.size(), 2);

	// Find Pool Heat and Spa Heat by label
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
			BOOST_CHECK(status.value() == HeaterStatuses::Off);
		}
		else if (label.value() == "Spa Heat")
		{
			found_spa_heat = true;
			auto status = heater->AuxillaryTraits.TryGet(HeaterStatusTrait{});
			BOOST_REQUIRE(status.has_value());
			BOOST_CHECK(status.value() == HeaterStatuses::Enabled);
		}
	}

	BOOST_CHECK(found_pool_heat);
	BOOST_CHECK(found_spa_heat);
}

BOOST_AUTO_TEST_CASE(Test_DualBodyShared_ChlorinatorWithSharedBody)
{
	Scenarios::PopulateDualBodyShared(*DataHub());

	auto chlorinators = DataHub()->Chlorinators();
	BOOST_REQUIRE_EQUAL(chlorinators.size(), 1);

	auto body = chlorinators[0]->AuxillaryTraits.TryGet(BodyOfWaterTrait{});
	BOOST_REQUIRE(body.has_value());
	BOOST_CHECK(body.value() == BodyOfWaterIds::Shared);

	auto percentage = chlorinators[0]->AuxillaryTraits.TryGet(GeneratingPercentageTrait{});
	BOOST_REQUIRE(percentage.has_value());
	BOOST_CHECK_EQUAL(percentage.value(), 60);
}

BOOST_AUTO_TEST_CASE(Test_DualBodyShared_ThreeAuxillaries)
{
	Scenarios::PopulateDualBodyShared(*DataHub());

	auto auxillaries = DataHub()->Auxillaries();
	BOOST_CHECK_EQUAL(auxillaries.size(), 3);
}

BOOST_AUTO_TEST_CASE(Test_DualBodyShared_TotalDeviceCount)
{
	Scenarios::PopulateDualBodyShared(*DataHub());

	// 1 pump + 2 heaters + 3 aux + 1 chlorinator = 7
	auto total = DataHub()->Pumps().size()
		+ DataHub()->Heaters().size()
		+ DataHub()->Auxillaries().size()
		+ DataHub()->Chlorinators().size();
	BOOST_CHECK_EQUAL(total, 7);
}

BOOST_AUTO_TEST_SUITE_END()
