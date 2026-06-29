#include <memory>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "kernel/body_of_water_ids.h"
#include "kernel/circulation.h"
#include "kernel/data_hub.h"
#include "kernel/hub_events/data_hub_config_event.h"
#include "kernel/hub_events/data_hub_config_event_circulation.h"
#include "kernel/hub_events/hub_eventtypes.h"
#include "kernel/pool_configurations.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Kernel;

BOOST_AUTO_TEST_SUITE(DataHub_Circulation_TestSuite)

BOOST_AUTO_TEST_CASE(SetCirculationMode_EmitsOnChange_DualBody)
{
	DataHub hub;
	hub.ApplyPoolConfiguration(PoolConfigurations::DualBody_SharedEquipment, ConfigurationSource::UserSpecified);

	std::vector<std::shared_ptr<DataHub_ConfigEvent_Circulation>> events;
	hub.ConfigUpdateSignal.connect(
		[&events](const std::shared_ptr<DataHub_ConfigEvent>& ev)
		{
			if (ev && ev->Type() == Hub_EventTypes::Circulation)
			{
				events.push_back(std::dynamic_pointer_cast<DataHub_ConfigEvent_Circulation>(ev));
			}
		});

	// Pool -> Spa is a real change: exactly one event, Spa is active.
	hub.SetCirculationMode(CirculationModes::Spa);
	BOOST_REQUIRE_EQUAL(events.size(), 1U);
	BOOST_CHECK(events.front()->Mode() == CirculationModes::Spa);

	auto spa = hub.GetBody(BodyOfWaterIds::Spa);
	auto pool = hub.GetBody(BodyOfWaterIds::Pool);
	BOOST_REQUIRE(spa.has_value());
	BOOST_REQUIRE(pool.has_value());
	BOOST_CHECK(spa->get().IsActive());
	BOOST_CHECK(!pool->get().IsActive());

	const auto payload = events.front()->ToJSON();
	BOOST_CHECK_EQUAL(payload["mode"], "Spa");
	BOOST_CHECK_EQUAL(payload["spa_mode"], true);
	BOOST_CHECK_EQUAL(payload["active_body"], "Spa");
	BOOST_REQUIRE(payload["bodies"].is_array());
	BOOST_CHECK_EQUAL(payload["bodies"].size(), 2U);
}

BOOST_AUTO_TEST_CASE(SetCirculationMode_NoEmitWhenUnchanged)
{
	DataHub hub;
	hub.ApplyPoolConfiguration(PoolConfigurations::DualBody_SharedEquipment, ConfigurationSource::UserSpecified);

	int circulation_events = 0;
	hub.ConfigUpdateSignal.connect(
		[&circulation_events](const std::shared_ptr<DataHub_ConfigEvent>& ev)
		{
			if (ev && ev->Type() == Hub_EventTypes::Circulation)
			{
				++circulation_events;
			}
		});

	// Default mode after ApplyPoolConfiguration is Pool with Pool active. Re-asserting Pool
	// changes nothing and must NOT fan out an event (callers poll this every status frame).
	hub.SetCirculationMode(CirculationModes::Pool);
	BOOST_CHECK_EQUAL(circulation_events, 0);

	// First transition to Spa fires once; a second identical Spa call must be silent.
	hub.SetCirculationMode(CirculationModes::Spa);
	hub.SetCirculationMode(CirculationModes::Spa);
	BOOST_CHECK_EQUAL(circulation_events, 1);
}

BOOST_AUTO_TEST_SUITE_END()
