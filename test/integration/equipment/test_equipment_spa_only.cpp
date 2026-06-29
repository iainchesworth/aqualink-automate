#include <boost/test/unit_test.hpp>

#include "kernel/auxillary_devices/heater_status.h"
#include "kernel/auxillary_devices/pump_status.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/body_of_water_ids.h"
#include "kernel/circulation.h"
#include "kernel/pool_configurations.h"

#include "support/integration_test_pipeline.h"
#include "support/integration_test_scenarios.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Kernel;
using namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes;
using namespace AqualinkAutomate::Test;

//=============================================================================
// Spa-Only (SingleBody) Equipment Configuration Integration Tests
//=============================================================================

BOOST_FIXTURE_TEST_SUITE(TestSuite_Integration_Equipment_SpaOnly, IntegrationPipeline)

BOOST_AUTO_TEST_CASE(Test_SpaOnly_PoolConfigurationApplied)
{
	Scenarios::PopulateSpaOnly(*DataHub());

	BOOST_CHECK_EQUAL(static_cast<int>(DataHub()->PoolConfiguration), static_cast<int>(PoolConfigurations::SingleBody));
}

BOOST_AUTO_TEST_CASE(Test_SpaOnly_SpaBodyExistsAndIsActive)
{
	Scenarios::PopulateSpaOnly(*DataHub());

	// A spa-only install must materialise a Spa body (not a Pool body) and mark it active -
	// it is the system's single, always-active body.
	BOOST_REQUIRE_EQUAL(DataHub()->Bodies().size(), 1);

	auto spa_body = DataHub()->GetBody(BodyOfWaterIds::Spa);
	BOOST_REQUIRE(spa_body.has_value());
	BOOST_CHECK(spa_body->get().IsActive());

	auto pool_body = DataHub()->GetBody(BodyOfWaterIds::Pool);
	BOOST_CHECK(!pool_body.has_value());
}

BOOST_AUTO_TEST_CASE(Test_SpaOnly_SpaTempResolvesViaBody)
{
	Scenarios::PopulateSpaOnly(*DataHub());

	// With a Spa body present, SpaTemp()/SpaTempSetpoint() must resolve through the body.
	auto spa_body = DataHub()->GetBody(BodyOfWaterIds::Spa);
	BOOST_REQUIRE(spa_body.has_value());

	auto body_temp = spa_body->get().CurrentTemp();
	BOOST_REQUIRE(body_temp.has_value());
	BOOST_CHECK_CLOSE(body_temp->InFahrenheit().value(), 101.0, 1.0);

	auto spa_temp = DataHub()->SpaTemp();
	BOOST_REQUIRE(spa_temp.has_value());
	BOOST_CHECK_CLOSE(spa_temp->InFahrenheit().value(), 101.0, 1.0);
}

BOOST_AUTO_TEST_CASE(Test_SpaOnly_CirculationModeIsSpa)
{
	Scenarios::PopulateSpaOnly(*DataHub());

	BOOST_CHECK(DataHub()->CirculationMode == CirculationModes::Spa);
	BOOST_CHECK(DataHub()->SpaMode());
}

BOOST_AUTO_TEST_CASE(Test_SpaOnly_TemperaturesSet)
{
	Scenarios::PopulateSpaOnly(*DataHub());

	auto air_temp = DataHub()->AirTemp();
	BOOST_REQUIRE(air_temp.has_value());
	BOOST_CHECK_CLOSE(air_temp->InFahrenheit().value(), 72.0, 1.0);

	auto spa_temp = DataHub()->SpaTemp();
	BOOST_REQUIRE(spa_temp.has_value());
	BOOST_CHECK_CLOSE(spa_temp->InFahrenheit().value(), 101.0, 1.0);

	auto spa_setpoint = DataHub()->SpaTempSetpoint();
	BOOST_REQUIRE(spa_setpoint.has_value());
	BOOST_CHECK_CLOSE(spa_setpoint->InFahrenheit().value(), 102.0, 1.0);
}

BOOST_AUTO_TEST_CASE(Test_SpaOnly_HeaterIsHeating)
{
	Scenarios::PopulateSpaOnly(*DataHub());

	auto heaters = DataHub()->Heaters();
	BOOST_REQUIRE_EQUAL(heaters.size(), 1);

	auto label = heaters[0]->AuxillaryTraits.TryGet(LabelTrait{});
	BOOST_REQUIRE(label.has_value());
	BOOST_CHECK_EQUAL(label.value(), "Spa Heat");

	auto status = heaters[0]->AuxillaryTraits.TryGet(HeaterStatusTrait{});
	BOOST_REQUIRE(status.has_value());
	BOOST_CHECK(status.value() == HeaterStatuses::Heating);
}

BOOST_AUTO_TEST_CASE(Test_SpaOnly_PumpCreated)
{
	Scenarios::PopulateSpaOnly(*DataHub());

	auto pumps = DataHub()->Pumps();
	BOOST_REQUIRE_EQUAL(pumps.size(), 1);

	auto status = pumps[0]->AuxillaryTraits.TryGet(PumpStatusTrait{});
	BOOST_REQUIRE(status.has_value());
	BOOST_CHECK(status.value() == PumpStatuses::Running);
}

BOOST_AUTO_TEST_CASE(Test_SpaOnly_AuxillariesCreated)
{
	Scenarios::PopulateSpaOnly(*DataHub());

	auto auxillaries = DataHub()->Auxillaries();
	BOOST_REQUIRE_EQUAL(auxillaries.size(), 2);
}

BOOST_AUTO_TEST_CASE(Test_SpaOnly_NoChlorinator)
{
	Scenarios::PopulateSpaOnly(*DataHub());

	auto chlorinators = DataHub()->Chlorinators();
	BOOST_CHECK_EQUAL(chlorinators.size(), 0);
}

BOOST_AUTO_TEST_CASE(Test_SpaOnly_TotalDeviceCount)
{
	Scenarios::PopulateSpaOnly(*DataHub());

	// 1 pump + 1 heater + 2 aux = 4
	auto total = DataHub()->Pumps().size()
		+ DataHub()->Heaters().size()
		+ DataHub()->Auxillaries().size()
		+ DataHub()->Chlorinators().size();
	BOOST_CHECK_EQUAL(total, 4);
}

BOOST_AUTO_TEST_SUITE_END()
