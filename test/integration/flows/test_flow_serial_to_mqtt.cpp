#include <boost/test/unit_test.hpp>

#include <string>

#include <nlohmann/json.hpp>

#include "kernel/auxillary_devices/chlorinator_status.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "mqtt/mqtt_client.h"

#include "support/integration_test_pipeline.h"
#include "support/integration_test_scenarios.h"
#include "support/unit_test_mqtt_support.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Kernel;
using namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes;
using namespace AqualinkAutomate::Test;

//=============================================================================
// Serial → DataHub → MQTT publish queue flow tests
//
// These tests verify that equipment state populated in the DataHub correctly
// produces MQTT publish messages via the MqttHub serialization layer.
// We use the MqttClient publish queue to capture outbound messages without
// needing a real MQTT broker connection.
//=============================================================================

BOOST_FIXTURE_TEST_SUITE(TestSuite_Integration_Flow_SerialToMqtt, IntegrationPipeline)

BOOST_AUTO_TEST_CASE(Test_Flow_PoolOnly_MqttPublishQueuePopulated)
{
	Scenarios::PopulatePoolOnly(*DataHub());
	PublishAllMqttStatus();

	// MqttHub.Start() + PublishAllStatus() should queue messages
	// even without a broker connection (they go into the client's publish queue).
	BOOST_CHECK_GT(MqttCapture().Count(), 0);
}

BOOST_AUTO_TEST_CASE(Test_Flow_PoolOnly_TemperatureTopicPublished)
{
	Scenarios::PopulatePoolOnly(*DataHub());
	PublishAllMqttStatus();

	auto temp_msgs = MqttCapture().MessagesForTopic("pool/temperatures");
	BOOST_REQUIRE_GT(temp_msgs.size(), 0);

	// Parse the payload and verify pool temp is present
	auto payload = nlohmann::json::parse(temp_msgs[0].payload);
	BOOST_CHECK(payload.contains("pool"));
	BOOST_CHECK(payload.contains("air"));
}

BOOST_AUTO_TEST_CASE(Test_Flow_PoolOnly_ChemistryTopicPublished)
{
	Scenarios::PopulatePoolOnly(*DataHub());
	PublishAllMqttStatus();

	auto chem_msgs = MqttCapture().MessagesForTopic("pool/chemistry");
	BOOST_REQUIRE_GT(chem_msgs.size(), 0);

	auto payload = nlohmann::json::parse(chem_msgs[0].payload);
	BOOST_REQUIRE(payload.contains("salt"));
	BOOST_CHECK(payload["salt"].contains("value_ppm"));
	BOOST_CHECK_EQUAL(payload["salt"]["value_ppm"].get<int>(), 3200);
}

BOOST_AUTO_TEST_CASE(Test_Flow_PoolOnly_CirculationTopicPublished)
{
	Scenarios::PopulatePoolOnly(*DataHub());
	PublishAllMqttStatus();

	auto circ_msgs = MqttCapture().MessagesForTopic("pool/circulation");
	BOOST_REQUIRE_GT(circ_msgs.size(), 0);

	auto payload = nlohmann::json::parse(circ_msgs[0].payload);
	BOOST_CHECK(payload.contains("mode"));
}

BOOST_AUTO_TEST_CASE(Test_Flow_PoolOnly_ConfigurationTopicPublished)
{
	Scenarios::PopulatePoolOnly(*DataHub());
	PublishAllMqttStatus();

	auto config_msgs = MqttCapture().MessagesForTopic("pool/configuration");
	BOOST_REQUIRE_GT(config_msgs.size(), 0);

	auto payload = nlohmann::json::parse(config_msgs[0].payload);
	BOOST_CHECK(payload.contains("pool_type"));
	// Controller operating mode (Normal/Service/TimeOut) is published for MQTT-only consumers.
	BOOST_CHECK(payload.contains("equipment_mode"));
}

BOOST_AUTO_TEST_CASE(Test_Flow_PoolOnly_DeviceTopicsPublished)
{
	Scenarios::PopulatePoolOnly(*DataHub());
	PublishAllMqttStatus();

	// Should have device topics for each device
	auto device_msgs = MqttCapture().MessagesForTopic("device/");
	BOOST_CHECK_GT(device_msgs.size(), 0);
}

BOOST_AUTO_TEST_CASE(Test_Flow_PoolOnly_ChlorinatorDevicePublished)
{
	Scenarios::PopulatePoolOnly(*DataHub());
	PublishAllMqttStatus();

	auto chlor_msgs = MqttCapture().MessagesForTopic("device/aquapure");
	BOOST_REQUIRE_GT(chlor_msgs.size(), 0);

	auto payload = nlohmann::json::parse(chlor_msgs[0].payload);
	BOOST_CHECK_EQUAL(payload["type"], "chlorinator");
	BOOST_CHECK(payload.contains("generating_percentage"));
	BOOST_CHECK_EQUAL(payload["generating_percentage"].get<int>(), 50);
}

BOOST_AUTO_TEST_CASE(Test_Flow_DualBody_BodyTemperatureTopicsPublished)
{
	Scenarios::PopulateDualBodyShared(*DataHub());
	PublishAllMqttStatus();

	auto pool_body_msgs = MqttCapture().MessagesForTopic("body/pool/temperature");
	BOOST_CHECK_GT(pool_body_msgs.size(), 0);

	auto spa_body_msgs = MqttCapture().MessagesForTopic("body/spa/temperature");
	BOOST_CHECK_GT(spa_body_msgs.size(), 0);
}

BOOST_AUTO_TEST_CASE(Test_Flow_SystemStatusPublished)
{
	Scenarios::PopulatePoolOnly(*DataHub());
	PublishAllMqttStatus();

	auto status_msgs = MqttCapture().MessagesForTopic("system/status");
	BOOST_CHECK_GT(status_msgs.size(), 0);
}

BOOST_AUTO_TEST_CASE(Test_Flow_StatisticsTopicsPublished)
{
	Scenarios::PopulatePoolOnly(*DataHub());
	PublishAllMqttStatus();

	BOOST_CHECK(MqttCapture().HasMessageForTopic("statistics/messages"));
	BOOST_CHECK(MqttCapture().HasMessageForTopic("statistics/bandwidth"));
	BOOST_CHECK(MqttCapture().HasMessageForTopic("statistics/latency"));
	BOOST_CHECK(MqttCapture().HasMessageForTopic("statistics/serial"));
}

BOOST_AUTO_TEST_SUITE_END()
