#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <nlohmann/json.hpp>

#include "mqtt/ha_discovery.h"
#include "mqtt/mqtt_client.h"
#include "kernel/data_hub.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "options/options_mqtt_options.h"
#include "support/unit_test_mqtt_support.h"

using namespace AqualinkAutomate;

namespace
{
	Options::Mqtt::MqttSettings MakeTestSettings()
	{
		auto s = Test::MakeMqttSettings();
		s.topic_prefix = "aqualink";
		s.home_assistant_enabled = true;
		s.ha_discovery_prefix = "homeassistant";
		s.ha_device_id = "aqualink_aqualink";
		return s;
	}
}

//=============================================================================
// HA Discovery Slugify tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_HaDiscovery_Slugify)

BOOST_AUTO_TEST_CASE(Test_Slugify_BasicConversion)
{
	BOOST_CHECK_EQUAL(Mqtt::HomeAssistantDiscovery::Slugify("Filter Pump"), "filter_pump");
}

BOOST_AUTO_TEST_CASE(Test_Slugify_AllLowercase)
{
	BOOST_CHECK_EQUAL(Mqtt::HomeAssistantDiscovery::Slugify("pool"), "pool");
}

BOOST_AUTO_TEST_CASE(Test_Slugify_AllUppercase)
{
	BOOST_CHECK_EQUAL(Mqtt::HomeAssistantDiscovery::Slugify("POOL HEATER"), "pool_heater");
}

BOOST_AUTO_TEST_CASE(Test_Slugify_MixedCaseWithHyphen)
{
	BOOST_CHECK_EQUAL(Mqtt::HomeAssistantDiscovery::Slugify("Spa-Heater"), "spa_heater");
}

BOOST_AUTO_TEST_CASE(Test_Slugify_WithDots)
{
	BOOST_CHECK_EQUAL(Mqtt::HomeAssistantDiscovery::Slugify("Pump.1"), "pump_1");
}

BOOST_AUTO_TEST_CASE(Test_Slugify_MultipleSpaces)
{
	BOOST_CHECK_EQUAL(Mqtt::HomeAssistantDiscovery::Slugify("Pool  Filter  Pump"), "pool_filter_pump");
}

BOOST_AUTO_TEST_CASE(Test_Slugify_TrailingSpace)
{
	BOOST_CHECK_EQUAL(Mqtt::HomeAssistantDiscovery::Slugify("Pump "), "pump");
}

BOOST_AUTO_TEST_CASE(Test_Slugify_EmptyString)
{
	BOOST_CHECK_EQUAL(Mqtt::HomeAssistantDiscovery::Slugify(""), "");
}

BOOST_AUTO_TEST_CASE(Test_Slugify_SingleWord)
{
	BOOST_CHECK_EQUAL(Mqtt::HomeAssistantDiscovery::Slugify("Chlorinator"), "chlorinator");
}

BOOST_AUTO_TEST_CASE(Test_Slugify_SpecialCharsStripped)
{
	BOOST_CHECK_EQUAL(Mqtt::HomeAssistantDiscovery::Slugify("Aux #1 (Pool)"), "aux_1_pool");
}

BOOST_AUTO_TEST_CASE(Test_Slugify_NumericInput)
{
	BOOST_CHECK_EQUAL(Mqtt::HomeAssistantDiscovery::Slugify("123"), "123");
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// HA Discovery UniqueId tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_HaDiscovery_UniqueId)

BOOST_AUTO_TEST_CASE(Test_UniqueId_Format)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	auto id = ha.UniqueId("pool_temp");
	BOOST_CHECK_EQUAL(id, "aqualink_aqualink_pool_temp");
}

BOOST_AUTO_TEST_CASE(Test_UniqueId_DifferentPrefix)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	settings.topic_prefix = "my_pool";
	settings.ha_device_id = "aqualink_my_pool";
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	auto id = ha.UniqueId("spa_temp");
	BOOST_CHECK_EQUAL(id, "aqualink_my_pool_spa_temp");
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// HA Discovery JSON object tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_HaDiscovery_JsonObjects)

BOOST_AUTO_TEST_CASE(Test_BuildDeviceObject_HasRequiredFields)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	auto device = ha.BuildDeviceObject();

	BOOST_CHECK(device.contains("identifiers"));
	BOOST_CHECK(device.contains("name"));
	BOOST_CHECK(device.contains("manufacturer"));
	BOOST_CHECK(device.contains("sw_version"));

	BOOST_CHECK_EQUAL(device["name"], "aqualink-automate");
	BOOST_CHECK_EQUAL(device["manufacturer"], "Jandy / Zodiac");

	auto identifiers = device["identifiers"];
	BOOST_REQUIRE(identifiers.is_array());
	BOOST_CHECK_EQUAL(identifiers.size(), 1);
	BOOST_CHECK_EQUAL(identifiers[0], "aqualink_aqualink");
}

BOOST_AUTO_TEST_CASE(Test_BuildDeviceObject_WithDataHub)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	auto data_hub = std::make_shared<Kernel::DataHub>();
	data_hub->EquipmentVersions.Set("Model", "RS-8");
	data_hub->EquipmentVersions.Set("Revision", "REV T.2");
	ha.ConnectDataHub(data_hub);

	auto device = ha.BuildDeviceObject();

	BOOST_CHECK(device.contains("model"));
	BOOST_CHECK_EQUAL(device["model"], "RS-8");
	BOOST_CHECK(device.contains("hw_version"));
	BOOST_CHECK_EQUAL(device["hw_version"], "REV T.2");
}

BOOST_AUTO_TEST_CASE(Test_BuildOriginObject_HasRequiredFields)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	auto origin = ha.BuildOriginObject();

	BOOST_CHECK(origin.contains("name"));
	BOOST_CHECK(origin.contains("sw_version"));
	BOOST_CHECK(origin.contains("support_url"));
	BOOST_CHECK_EQUAL(origin["name"], "aqualink-automate");
}

BOOST_AUTO_TEST_CASE(Test_BuildAvailability_HasCorrectStructure)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	auto availability = ha.BuildAvailability();

	BOOST_REQUIRE(availability.is_array());
	BOOST_REQUIRE_EQUAL(availability.size(), 1);

	auto entry = availability[0];
	BOOST_CHECK(entry.contains("topic"));
	BOOST_CHECK(entry.contains("payload_available"));
	BOOST_CHECK(entry.contains("payload_not_available"));
	BOOST_CHECK_EQUAL(entry["topic"], "aqualink/status/availability");
	BOOST_CHECK_EQUAL(entry["payload_available"], "online");
	BOOST_CHECK_EQUAL(entry["payload_not_available"], "offline");
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// HA Discovery topic tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_HaDiscovery_Topics)

BOOST_AUTO_TEST_CASE(Test_AvailabilityTopic)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	BOOST_CHECK_EQUAL(ha.AvailabilityTopic(), "aqualink/status/availability");
}

BOOST_AUTO_TEST_CASE(Test_TemperaturesTopic)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	BOOST_CHECK_EQUAL(ha.TemperaturesTopic(), "aqualink/pool/temperatures");
}

BOOST_AUTO_TEST_CASE(Test_ChemistryTopic)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	BOOST_CHECK_EQUAL(ha.ChemistryTopic(), "aqualink/pool/chemistry");
}

BOOST_AUTO_TEST_CASE(Test_CirculationTopic)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	BOOST_CHECK_EQUAL(ha.CirculationTopic(), "aqualink/pool/circulation");
}

BOOST_AUTO_TEST_CASE(Test_SystemStatusTopic)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	BOOST_CHECK_EQUAL(ha.SystemStatusTopic(), "aqualink/system/status");
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// HA Discovery publish flow tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_HaDiscovery_Publish)

BOOST_AUTO_TEST_CASE(Test_PublishDiscoveryConfigs_QueuesToClient)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	ha.PublishDiscoveryConfigs();

	auto& queue = Test::MqttClientPacketTest::GetPublishQueue(*client);

	// Device discovery publishes a single payload
	BOOST_REQUIRE_EQUAL(queue.size(), 1);

	// Discovery config should be retained
	BOOST_CHECK(queue[0].retain);

	// Verify discovery topic format
	BOOST_CHECK_EQUAL(queue[0].topic, "homeassistant/device/aqualink_aqualink/config");

	// Parse the payload and verify cmps has at least 13 components
	auto payload = nlohmann::json::parse(queue[0].payload);
	BOOST_REQUIRE(payload.contains("cmps"));
	BOOST_CHECK_GE(payload["cmps"].size(), 13);
}

BOOST_AUTO_TEST_CASE(Test_PublishDiscoveryConfigs_ValidJson)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	ha.PublishDiscoveryConfigs();

	auto& queue = Test::MqttClientPacketTest::GetPublishQueue(*client);
	BOOST_REQUIRE_EQUAL(queue.size(), 1);

	auto payload = nlohmann::json::parse(queue[0].payload);
	BOOST_REQUIRE(payload.contains("cmps"));

	for (auto& [key, cmp] : payload["cmps"].items())
	{
		BOOST_CHECK_MESSAGE(cmp.contains("p"), "Component missing 'p' (platform): " + key);
		BOOST_CHECK_MESSAGE(cmp.contains("name"), "Component missing 'name': " + key);
		BOOST_CHECK_MESSAGE(cmp.contains("unique_id"), "Component missing 'unique_id': " + key);
		BOOST_CHECK_MESSAGE(cmp.contains("state_topic"), "Component missing 'state_topic': " + key);
	}
}

BOOST_AUTO_TEST_CASE(Test_PublishDiscoveryConfigs_TemperatureSensorsHaveCorrectClass)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	ha.PublishDiscoveryConfigs();

	auto& queue = Test::MqttClientPacketTest::GetPublishQueue(*client);
	BOOST_REQUIRE_EQUAL(queue.size(), 1);

	auto payload = nlohmann::json::parse(queue[0].payload);
	auto& cmps = payload["cmps"];

	int temp_sensor_count = 0;
	int temp_number_count = 0;
	for (auto& [key, cmp] : cmps.items())
	{
		if (cmp.contains("device_class") && cmp["device_class"] == "temperature")
		{
			BOOST_CHECK_EQUAL(cmp["unit_of_measurement"], "\u00B0C");
			BOOST_CHECK_EQUAL(cmp["state_topic"], "aqualink/pool/temperatures");

			auto platform = cmp["p"].get<std::string>();
			if (platform == "sensor")
			{
				++temp_sensor_count;
				BOOST_CHECK_EQUAL(cmp["state_class"], "measurement");
			}
			else if (platform == "number")
			{
				++temp_number_count;
				BOOST_CHECK_MESSAGE(cmp.contains("command_topic"), "number entity missing 'command_topic': " + key);
				BOOST_CHECK_MESSAGE(cmp.contains("min"), "number entity missing 'min': " + key);
				BOOST_CHECK_MESSAGE(cmp.contains("max"), "number entity missing 'max': " + key);
				BOOST_CHECK_MESSAGE(cmp.contains("step"), "number entity missing 'step': " + key);
			}
		}
	}

	// Should have 4 temperature sensors (pool, spa, air, freeze_protect) + 2 number entities (pool_setpoint, spa_setpoint)
	BOOST_CHECK_EQUAL(temp_sensor_count, 4);
	BOOST_CHECK_EQUAL(temp_number_count, 2);
}

BOOST_AUTO_TEST_CASE(Test_PublishDiscoveryConfigs_BinarySensorsHavePayloadOnOff)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	ha.PublishDiscoveryConfigs();

	auto& queue = Test::MqttClientPacketTest::GetPublishQueue(*client);
	BOOST_REQUIRE_EQUAL(queue.size(), 1);

	auto payload = nlohmann::json::parse(queue[0].payload);
	auto& cmps = payload["cmps"];

	for (auto& [key, cmp] : cmps.items())
	{
		if (cmp.contains("p") && cmp["p"] == "binary_sensor")
		{
			BOOST_CHECK_MESSAGE(cmp.contains("payload_on"),
				"binary_sensor component missing 'payload_on': " + key);
			BOOST_CHECK_MESSAGE(cmp.contains("payload_off"),
				"binary_sensor component missing 'payload_off': " + key);
		}
	}
}

BOOST_AUTO_TEST_CASE(Test_PublishDiscoveryConfigs_SwitchEntitiesHaveCommandTopic)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	// Create a data hub with a pump device so we get a switch entity
	auto data_hub = std::make_shared<Kernel::DataHub>();
	ha.ConnectDataHub(data_hub);

	auto pump = std::make_shared<Kernel::AuxillaryDevice>();
	pump->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait{}, Kernel::AuxillaryTraitsTypes::AuxillaryTypes::Pump);
	pump->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::LabelTrait{}, std::string("Filter Pump"));
	pump->AuxillaryTraits.Set(Kernel::AuxillaryTraitsTypes::PumpStatusTrait{}, Kernel::PumpStatuses::Off);
	data_hub->Devices.Add(pump);

	ha.PublishDiscoveryConfigs();

	auto& queue = Test::MqttClientPacketTest::GetPublishQueue(*client);
	BOOST_REQUIRE_EQUAL(queue.size(), 1);

	auto payload = nlohmann::json::parse(queue[0].payload);
	auto& cmps = payload["cmps"];

	int switch_count = 0;
	for (auto& [key, cmp] : cmps.items())
	{
		if (cmp.contains("p") && cmp["p"] == "switch")
		{
			++switch_count;
			BOOST_CHECK_MESSAGE(cmp.contains("command_topic"),
				"switch component missing 'command_topic': " + key);
			BOOST_CHECK_MESSAGE(cmp.contains("state_on"),
				"switch component missing 'state_on': " + key);
			BOOST_CHECK_MESSAGE(cmp.contains("state_off"),
				"switch component missing 'state_off': " + key);
			BOOST_CHECK_MESSAGE(cmp.contains("payload_on"),
				"switch component missing 'payload_on': " + key);
			BOOST_CHECK_MESSAGE(cmp.contains("payload_off"),
				"switch component missing 'payload_off': " + key);
			BOOST_CHECK_EQUAL(cmp["payload_on"], "ON");
			BOOST_CHECK_EQUAL(cmp["payload_off"], "OFF");
		}
	}

	BOOST_CHECK_GE(switch_count, 1);
}

BOOST_AUTO_TEST_CASE(Test_PublishOnline_PublishesRetainedOnline)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	ha.PublishOnline();

	auto& queue = Test::MqttClientPacketTest::GetPublishQueue(*client);
	BOOST_REQUIRE_EQUAL(queue.size(), 1);
	BOOST_CHECK_EQUAL(queue[0].topic, "aqualink/status/availability");
	BOOST_CHECK_EQUAL(queue[0].payload, "online");
	BOOST_CHECK_EQUAL(queue[0].retain, true);
}

BOOST_AUTO_TEST_CASE(Test_PublishDeviceStates_NoDataHub_NoCrash)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	// No DataHub connected - should not crash
	BOOST_CHECK_NO_THROW(ha.PublishDeviceStates());

	auto& queue = Test::MqttClientPacketTest::GetPublishQueue(*client);
	BOOST_CHECK_EQUAL(queue.size(), 0);
}

BOOST_AUTO_TEST_CASE(Test_PublishDeviceStates_EmptyDataHub_NoCrash)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	auto data_hub = std::make_shared<Kernel::DataHub>();
	ha.ConnectDataHub(data_hub);

	// Empty data hub (no devices) - should not crash
	BOOST_CHECK_NO_THROW(ha.PublishDeviceStates());
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// HA Device Discovery payload format tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_HaDiscovery_DevicePayload)

BOOST_AUTO_TEST_CASE(Test_DeviceDiscoveryTopic_Format)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	ha.PublishDiscoveryConfigs();

	auto& queue = Test::MqttClientPacketTest::GetPublishQueue(*client);
	BOOST_REQUIRE_EQUAL(queue.size(), 1);
	BOOST_CHECK_EQUAL(queue[0].topic, "homeassistant/device/aqualink_aqualink/config");
}

BOOST_AUTO_TEST_CASE(Test_DeviceDiscoveryPayload_HasRootFields)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	ha.PublishDiscoveryConfigs();

	auto& queue = Test::MqttClientPacketTest::GetPublishQueue(*client);
	BOOST_REQUIRE_EQUAL(queue.size(), 1);

	auto payload = nlohmann::json::parse(queue[0].payload);
	BOOST_CHECK(payload.contains("dev"));
	BOOST_CHECK(payload.contains("o"));
	BOOST_CHECK(payload.contains("availability"));
	BOOST_CHECK(payload.contains("cmps"));
}

BOOST_AUTO_TEST_CASE(Test_DeviceDiscoveryPayload_NoDuplicateDeviceInComponents)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	ha.PublishDiscoveryConfigs();

	auto& queue = Test::MqttClientPacketTest::GetPublishQueue(*client);
	BOOST_REQUIRE_EQUAL(queue.size(), 1);

	auto payload = nlohmann::json::parse(queue[0].payload);
	auto& cmps = payload["cmps"];

	for (auto& [key, cmp] : cmps.items())
	{
		BOOST_CHECK_MESSAGE(!cmp.contains("device"),
			"Component should not contain 'device' (it's at root level): " + key);
		BOOST_CHECK_MESSAGE(!cmp.contains("origin"),
			"Component should not contain 'origin' (it's at root level): " + key);
		BOOST_CHECK_MESSAGE(!cmp.contains("availability"),
			"Component should not contain 'availability' (it's at root level): " + key);
	}
}

BOOST_AUTO_TEST_CASE(Test_DeviceDiscoveryPayload_AllComponentsHavePlatform)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	ha.PublishDiscoveryConfigs();

	auto& queue = Test::MqttClientPacketTest::GetPublishQueue(*client);
	BOOST_REQUIRE_EQUAL(queue.size(), 1);

	auto payload = nlohmann::json::parse(queue[0].payload);
	auto& cmps = payload["cmps"];

	for (auto& [key, cmp] : cmps.items())
	{
		BOOST_CHECK_MESSAGE(cmp.contains("p"),
			"Component missing 'p' (platform): " + key);

		auto platform = cmp["p"].get<std::string>();
		BOOST_CHECK_MESSAGE(platform == "sensor" || platform == "binary_sensor" || platform == "number" || platform == "switch",
			std::string("Component has unexpected platform '").append(platform).append("': ").append(key));
	}
}

BOOST_AUTO_TEST_CASE(Test_CustomDeviceId)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	settings.ha_device_id = "custom_pool_1";
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	ha.PublishDiscoveryConfigs();

	auto& queue = Test::MqttClientPacketTest::GetPublishQueue(*client);
	BOOST_REQUIRE_EQUAL(queue.size(), 1);

	// Verify topic uses custom device ID
	BOOST_CHECK_EQUAL(queue[0].topic, "homeassistant/device/custom_pool_1/config");

	// Verify device identifier uses custom device ID
	auto payload = nlohmann::json::parse(queue[0].payload);
	auto& identifiers = payload["dev"]["identifiers"];
	BOOST_REQUIRE(identifiers.is_array());
	BOOST_CHECK_EQUAL(identifiers[0], "custom_pool_1");

	// Verify unique_ids use custom device ID prefix
	auto& cmps = payload["cmps"];
	BOOST_REQUIRE(cmps.contains("pool_temp"));
	BOOST_CHECK_EQUAL(cmps["pool_temp"]["unique_id"], "custom_pool_1_pool_temp");
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// MqttClient retain flag tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_MqttClient_Retain)

BOOST_AUTO_TEST_CASE(Test_Publish_DefaultRetainIsFalse)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);

	client->Publish("test/topic", "payload");

	auto& queue = Test::MqttClientPacketTest::GetPublishQueue(*client);
	BOOST_REQUIRE_EQUAL(queue.size(), 1);
	BOOST_CHECK_EQUAL(queue[0].retain, false);
}

BOOST_AUTO_TEST_CASE(Test_Publish_RetainTrue)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);

	client->Publish("test/topic", "payload", true);

	auto& queue = Test::MqttClientPacketTest::GetPublishQueue(*client);
	BOOST_REQUIRE_EQUAL(queue.size(), 1);
	BOOST_CHECK_EQUAL(queue[0].retain, true);
}

BOOST_AUTO_TEST_CASE(Test_EncodePublish_RetainBitClear)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	Mqtt::MqttClient client(ioc, settings);

	auto packet = Test::MqttClientPacketTest::EncodePublish(client, "topic", "payload", false);
	BOOST_REQUIRE_GT(packet.size(), 0);

	// First byte: 0x30 for PUBLISH with retain=0
	BOOST_CHECK_EQUAL(packet[0] & 0x01, 0x00);
	BOOST_CHECK_EQUAL(packet[0] & 0xF0, 0x30);
}

BOOST_AUTO_TEST_CASE(Test_EncodePublish_RetainBitSet)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	Mqtt::MqttClient client(ioc, settings);

	auto packet = Test::MqttClientPacketTest::EncodePublish(client, "topic", "payload", true);
	BOOST_REQUIRE_GT(packet.size(), 0);

	// First byte: 0x31 for PUBLISH with retain=1
	BOOST_CHECK_EQUAL(packet[0] & 0x01, 0x01);
	BOOST_CHECK_EQUAL(packet[0] & 0xF0, 0x30);
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// MqttClient LWT tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_MqttClient_LWT)

BOOST_AUTO_TEST_CASE(Test_SetWill_StoresConfig)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	Mqtt::MqttClient client(ioc, settings);

	BOOST_CHECK(!client.GetWill().has_value());

	client.SetWill("test/status/availability", "offline", true);

	auto& will = client.GetWill();
	BOOST_REQUIRE(will.has_value());
	BOOST_CHECK_EQUAL(will->topic, "test/status/availability");
	BOOST_CHECK_EQUAL(will->payload, "offline");
	BOOST_CHECK_EQUAL(will->retain, true);
}

BOOST_AUTO_TEST_CASE(Test_EncodeConnect_WithoutWill_NoWillFlag)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	settings.username = "";
	settings.password = "";
	Mqtt::MqttClient client(ioc, settings);

	auto packet = Test::MqttClientPacketTest::EncodeConnect(client);

	// Find the connect flags byte. CONNECT packet layout:
	// Fixed header: type(1) + remaining_length(1-4)
	// Variable header: protocol name length(2) + "MQTT"(4) + protocol_level(1) + flags(1) + keepalive(2)
	// The flags byte is at offset: fixed_header_size + 2 + 4 + 1 = fixed_header_size + 7
	// Fixed header is: 0x10 + remaining_length (1 byte for small packets)
	BOOST_REQUIRE_GT(packet.size(), 9);

	uint8_t flags = packet[9]; // 1 (type) + 1 (rem_len) + 2 (str_len) + 4 ("MQTT") + 1 (protocol_level) = 9

	// Will Flag (bit 2) should NOT be set
	BOOST_CHECK_EQUAL(flags & 0x04, 0x00);
	// Will Retain (bit 5) should NOT be set
	BOOST_CHECK_EQUAL(flags & 0x20, 0x00);
}

BOOST_AUTO_TEST_CASE(Test_EncodeConnect_WithWill_SetsFlags)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	settings.username = "";
	settings.password = "";
	Mqtt::MqttClient client(ioc, settings);

	client.SetWill("test/lwt", "offline", true);

	auto packet = Test::MqttClientPacketTest::EncodeConnect(client);

	BOOST_REQUIRE_GT(packet.size(), 9);

	uint8_t flags = packet[9];

	// Will Flag (bit 2) should be set
	BOOST_CHECK_EQUAL(flags & 0x04, 0x04);
	// Will Retain (bit 5) should be set
	BOOST_CHECK_EQUAL(flags & 0x20, 0x20);
	// Clean Session (bit 1) should still be set
	BOOST_CHECK_EQUAL(flags & 0x02, 0x02);
}

BOOST_AUTO_TEST_CASE(Test_EncodeConnect_WithWill_NoRetain_RetainBitClear)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	settings.username = "";
	settings.password = "";
	Mqtt::MqttClient client(ioc, settings);

	client.SetWill("test/lwt", "offline", false);

	auto packet = Test::MqttClientPacketTest::EncodeConnect(client);

	BOOST_REQUIRE_GT(packet.size(), 9);

	uint8_t flags = packet[9];

	// Will Flag (bit 2) should be set
	BOOST_CHECK_EQUAL(flags & 0x04, 0x04);
	// Will Retain (bit 5) should NOT be set
	BOOST_CHECK_EQUAL(flags & 0x20, 0x00);
}

BOOST_AUTO_TEST_CASE(Test_EncodeConnect_WithWill_ContainsWillTopic)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	settings.username = "";
	settings.password = "";
	Mqtt::MqttClient client(ioc, settings);

	client.SetWill("test/will/topic", "will_payload", true);

	auto packet = Test::MqttClientPacketTest::EncodeConnect(client);

	// The will topic should appear somewhere in the packet
	std::string packet_str(packet.begin(), packet.end());
	BOOST_CHECK_MESSAGE(
		packet_str.find("test/will/topic") != std::string::npos,
		"CONNECT packet should contain the will topic");
	BOOST_CHECK_MESSAGE(
		packet_str.find("will_payload") != std::string::npos,
		"CONNECT packet should contain the will payload");
}

BOOST_AUTO_TEST_CASE(Test_EncodeConnect_WillBeforeUsername)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	settings.username = "testuser";
	settings.password = "testpass";
	Mqtt::MqttClient client(ioc, settings);

	client.SetWill("test/lwt", "offline", true);

	auto packet = Test::MqttClientPacketTest::EncodeConnect(client);

	// Per MQTT 3.1.1 section 3.1.3: payload order is
	// Client ID, Will Topic, Will Message, Username, Password
	std::string packet_str(packet.begin(), packet.end());
	auto will_pos = packet_str.find("test/lwt");
	auto user_pos = packet_str.find("testuser");

	BOOST_REQUIRE_NE(will_pos, std::string::npos);
	BOOST_REQUIRE_NE(user_pos, std::string::npos);
	BOOST_CHECK_LT(will_pos, user_pos);
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// MqttSettings HA defaults tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_MqttSettings_HA)

BOOST_AUTO_TEST_CASE(Test_MqttSettings_HaDefaultsDisabled)
{
	Options::Mqtt::MqttSettings settings;
	BOOST_CHECK_EQUAL(settings.home_assistant_enabled, false);
	BOOST_CHECK_EQUAL(settings.ha_discovery_prefix, "homeassistant");
	BOOST_CHECK(settings.ha_device_id.empty());
}

BOOST_AUTO_TEST_CASE(Test_MqttSettings_HaEnabled)
{
	Options::Mqtt::MqttSettings settings;
	settings.home_assistant_enabled = true;
	settings.ha_discovery_prefix = "ha_test";

	BOOST_CHECK_EQUAL(settings.home_assistant_enabled, true);
	BOOST_CHECK_EQUAL(settings.ha_discovery_prefix, "ha_test");
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// HA Discovery config discovery prefix tests
//=============================================================================

BOOST_AUTO_TEST_SUITE(TestSuite_HaDiscovery_CustomPrefix)

BOOST_AUTO_TEST_CASE(Test_CustomDiscoveryPrefix)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	settings.ha_discovery_prefix = "my_ha";
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	ha.PublishDiscoveryConfigs();

	auto& queue = Test::MqttClientPacketTest::GetPublishQueue(*client);
	BOOST_REQUIRE_EQUAL(queue.size(), 1);

	BOOST_CHECK_MESSAGE(
		queue[0].topic.find("my_ha/device/") == 0,
		"Discovery topic should start with 'my_ha/device/': " + queue[0].topic);
}

BOOST_AUTO_TEST_CASE(Test_CustomTopicPrefix)
{
	boost::asio::io_context ioc;
	auto settings = MakeTestSettings();
	settings.topic_prefix = "mypool";
	settings.ha_device_id = "aqualink_mypool";
	auto client = std::make_shared<Mqtt::MqttClient>(ioc, settings);
	Mqtt::HomeAssistantDiscovery ha(client, settings);

	ha.PublishDiscoveryConfigs();

	auto& queue = Test::MqttClientPacketTest::GetPublishQueue(*client);
	BOOST_REQUIRE_EQUAL(queue.size(), 1);

	// Parse the single payload and check state_topic in each component
	auto payload = nlohmann::json::parse(queue[0].payload);
	auto& cmps = payload["cmps"];

	for (auto& [key, cmp] : cmps.items())
	{
		auto state_topic = cmp["state_topic"].get<std::string>();
		BOOST_CHECK_MESSAGE(
			state_topic.find("mypool/") == 0,
			"State topic should use custom prefix 'mypool/': " + state_topic);
	}
}

BOOST_AUTO_TEST_SUITE_END()
