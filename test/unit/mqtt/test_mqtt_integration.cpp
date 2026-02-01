#include <boost/test/unit_test.hpp>

#include <chrono>
#include <future>
#include <memory>

#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

#include "mqtt/mqtt_integration.h"
#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "options/options_mqtt_options.h"
//#include "mocks/mock_mqtt_broker.h"

using namespace AqualinkAutomate;
//using namespace AqualinkAutomate::Test::Mocks;
using namespace std::chrono_literals;
/*
BOOST_AUTO_TEST_SUITE(TestSuite_MqttIntegration)

struct MqttIntegrationFixture
{
	MqttIntegrationFixture() : ioc(), broker(ioc), data_hub(std::make_shared<Kernel::DataHub>()), 
							  equipment_hub(std::make_shared<Kernel::EquipmentHub>())
	{
		settings.is_enabled = true;
		settings.broker_host = "localhost";
		settings.broker_port = 1883;
		settings.client_id = "test-integration";
		settings.topic_prefix = "test";
		settings.auto_reconnect = false;
	}

	~MqttIntegrationFixture()
	{
		if (broker.is_running())
		{
			run_sync([this]() -> boost::cobalt::task<void> {
				co_await broker.stop();
			});
		}
	}

	template<typename F>
	auto run_sync(F&& f) -> decltype(f().result())
	{
		auto future = std::async(std::launch::async, [&]() {
			return boost::asio::co_spawn(ioc, std::forward<F>(f), boost::asio::use_future);
		});
		
		ioc.restart();
		ioc.run();
		
		return future.get().get();
	}

	boost::asio::io_context ioc;
	MockMqttBroker broker;
	Options::Mqtt::Settings settings;
	std::shared_ptr<Kernel::DataHub> data_hub;
	std::shared_ptr<Kernel::EquipmentHub> equipment_hub;
};

BOOST_FIXTURE_TEST_CASE(Test_MqttIntegration_ConstructionEnabled, MqttIntegrationFixture)
{
	BOOST_TEST_MESSAGE("Testing MQTT integration construction when enabled");
	
	BOOST_CHECK_NO_THROW({
		Mqtt::MqttIntegration integration(ioc, settings);
		BOOST_CHECK(integration.is_enabled());
	});
}

BOOST_FIXTURE_TEST_CASE(Test_MqttIntegration_ConstructionDisabled, MqttIntegrationFixture)
{
	BOOST_TEST_MESSAGE("Testing MQTT integration construction when disabled");
	
	settings.is_enabled = false;
	
	BOOST_CHECK_NO_THROW({
		Mqtt::MqttIntegration integration(ioc, settings);
		BOOST_CHECK(!integration.is_enabled());
	});
}

BOOST_FIXTURE_TEST_CASE(Test_MqttIntegration_StartStopEnabled, MqttIntegrationFixture)
{
	BOOST_TEST_MESSAGE("Testing MQTT integration start/stop when enabled");
	
	run_sync([this]() -> boost::cobalt::task<void> {
		co_await broker.start(1883);
	});
	
	Mqtt::MqttIntegration integration(ioc, settings);
	integration.connect_hubs(data_hub, equipment_hub);
	
	run_sync([&integration]() -> boost::cobalt::task<void> {
		co_await integration.start();
	});
	
	// Should have connected to broker
	auto connected_clients = broker.get_connected_clients();
	BOOST_CHECK_EQUAL(connected_clients.size(), 1);
	BOOST_CHECK_EQUAL(connected_clients[0], "test-integration");
	
	run_sync([&integration]() -> boost::cobalt::task<void> {
		co_await integration.stop();
	});
}

BOOST_FIXTURE_TEST_CASE(Test_MqttIntegration_StartStopDisabled, MqttIntegrationFixture)
{
	BOOST_TEST_MESSAGE("Testing MQTT integration start/stop when disabled");
	
	settings.is_enabled = false;
	
	Mqtt::MqttIntegration integration(ioc, settings);
	
	// Should complete without error but do nothing
	BOOST_CHECK_NO_THROW({
		run_sync([&integration]() -> boost::cobalt::task<void> {
			co_await integration.start();
		});
		
		run_sync([&integration]() -> boost::cobalt::task<void> {
			co_await integration.stop();
		});
	});
	
	// Should not have connected to any broker
	BOOST_CHECK(!integration.is_enabled());
}

BOOST_FIXTURE_TEST_CASE(Test_MqttIntegration_HubConnection, MqttIntegrationFixture)
{
	BOOST_TEST_MESSAGE("Testing MQTT integration hub connection");
	
	Mqtt::MqttIntegration integration(ioc, settings);
	
	// Should not throw when connecting hubs
	BOOST_CHECK_NO_THROW({
		integration.connect_hubs(data_hub, equipment_hub);
	});
	
	// Should be able to get MQTT hub reference
	auto mqtt_hub = integration.get_mqtt_hub();
	BOOST_CHECK(mqtt_hub != nullptr);
}

BOOST_FIXTURE_TEST_CASE(Test_MqttIntegration_DefaultStatusCommand, MqttIntegrationFixture)
{
	BOOST_TEST_MESSAGE("Testing MQTT integration default status command");
	
	run_sync([this]() -> boost::cobalt::task<void> {
		co_await broker.start(1883);
	});
	
	Mqtt::MqttIntegration integration(ioc, settings);
	integration.connect_hubs(data_hub, equipment_hub);
	
	run_sync([&integration]() -> boost::cobalt::task<void> {
		co_await integration.start();
	});
	
	broker.clear_published_messages();
	
	// Send status command
	run_sync([this]() -> boost::cobalt::task<void> {
		co_await broker.inject_message("test/command/status", "{}");
		
		// Give time for command processing
		boost::asio::steady_timer timer(ioc);
		timer.expires_after(100ms);
		co_await timer.async_wait(boost::asio::use_awaitable);
	});
	
	// Should have published status updates
	auto messages = broker.get_published_messages();
	BOOST_CHECK_GT(messages.size(), 0);
	
	// Should find status messages
	bool found_status_message = false;
	for (const auto& message : messages)
	{
		if (message.topic.find("test/status/") == 0)
		{
			found_status_message = true;
			break;
		}
	}
	
	BOOST_CHECK(found_status_message);
}

BOOST_FIXTURE_TEST_CASE(Test_MqttIntegration_DefaultDeviceCommand, MqttIntegrationFixture)
{
	BOOST_TEST_MESSAGE("Testing MQTT integration default device command");
	
	run_sync([this]() -> boost::cobalt::task<void> {
		co_await broker.start(1883);
	});
	
	Mqtt::MqttIntegration integration(ioc, settings);
	integration.connect_hubs(data_hub, equipment_hub);
	
	run_sync([&integration]() -> boost::cobalt::task<void> {
		co_await integration.start();
	});
	
	broker.clear_published_messages();
	
	// Send device command
	nlohmann::json device_command = {
		{"device_id", "pump1"},
		{"action", "on"}
	};
	
	run_sync([this, &device_command]() -> boost::cobalt::task<void> {
		co_await broker.inject_message("test/command/device", device_command.dump());
		
		boost::asio::steady_timer timer(ioc);
		timer.expires_after(100ms);
		co_await timer.async_wait(boost::asio::use_awaitable);
	});
	
	// Should have published command response
	auto messages = broker.get_published_messages();
	bool found_response = false;
	
	for (const auto& message : messages)
	{
		if (message.topic == "test/status/command/response")
		{
			nlohmann::json response = nlohmann::json::parse(message.payload);
			BOOST_CHECK_EQUAL(response["command"], "device");
			BOOST_CHECK_EQUAL(response["device_id"], "pump1");
			BOOST_CHECK_EQUAL(response["action"], "on");
			BOOST_CHECK_EQUAL(response["status"], "acknowledged");
			found_response = true;
			break;
		}
	}
	
	BOOST_CHECK(found_response);
}

BOOST_FIXTURE_TEST_CASE(Test_MqttIntegration_DeviceCommandMissingParameters, MqttIntegrationFixture)
{
	BOOST_TEST_MESSAGE("Testing MQTT integration device command with missing parameters");
	
	run_sync([this]() -> boost::cobalt::task<void> {
		co_await broker.start(1883);
	});
	
	Mqtt::MqttIntegration integration(ioc, settings);
	integration.connect_hubs(data_hub, equipment_hub);
	
	run_sync([&integration]() -> boost::cobalt::task<void> {
		co_await integration.start();
	});
	
	broker.clear_published_messages();
	
	// Send device command without required parameters
	nlohmann::json incomplete_command = {{"device_id", "pump1"}};  // Missing action
	
	// Should not throw - should handle gracefully
	BOOST_CHECK_NO_THROW({
		run_sync([this, &incomplete_command]() -> boost::cobalt::task<void> {
			co_await broker.inject_message("test/command/device", incomplete_command.dump());
			
			boost::asio::steady_timer timer(ioc);
			timer.expires_after(50ms);
			co_await timer.async_wait(boost::asio::use_awaitable);
		});
	});
	
	// Should not have published a response
	auto messages = broker.get_published_messages();
	bool found_response = false;
	
	for (const auto& message : messages)
	{
		if (message.topic == "test/status/command/response")
		{
			found_response = true;
			break;
		}
	}
	
	BOOST_CHECK(!found_response);
}

BOOST_FIXTURE_TEST_CASE(Test_MqttIntegration_DefaultTemperatureCommand, MqttIntegrationFixture)
{
	BOOST_TEST_MESSAGE("Testing MQTT integration default temperature command");
	
	run_sync([this]() -> boost::cobalt::task<void> {
		co_await broker.start(1883);
	});
	
	Mqtt::MqttIntegration integration(ioc, settings);
	integration.connect_hubs(data_hub, equipment_hub);
	
	run_sync([&integration]() -> boost::cobalt::task<void> {
		co_await integration.start();
	});
	
	broker.clear_published_messages();
	
	// Send temperature command
	nlohmann::json temp_command = {
		{"target", "pool"},
		{"temperature", 28.5},
		{"unit", "celsius"}
	};
	
	run_sync([this, &temp_command]() -> boost::cobalt::task<void> {
		co_await broker.inject_message("test/command/temperature", temp_command.dump());
		
		boost::asio::steady_timer timer(ioc);
		timer.expires_after(100ms);
		co_await timer.async_wait(boost::asio::use_awaitable);
	});
	
	// Should have published command response
	auto messages = broker.get_published_messages();
	bool found_response = false;
	
	for (const auto& message : messages)
	{
		if (message.topic == "test/status/command/response")
		{
			nlohmann::json response = nlohmann::json::parse(message.payload);
			BOOST_CHECK_EQUAL(response["command"], "temperature");
			BOOST_CHECK_EQUAL(response["target"], "pool");
			BOOST_CHECK_EQUAL(response["temperature"], 28.5);
			BOOST_CHECK_EQUAL(response["unit"], "celsius");
			BOOST_CHECK_EQUAL(response["status"], "acknowledged");
			found_response = true;
			break;
		}
	}
	
	BOOST_CHECK(found_response);
}

BOOST_FIXTURE_TEST_CASE(Test_MqttIntegration_TemperatureCommandDefaultUnit, MqttIntegrationFixture)
{
	BOOST_TEST_MESSAGE("Testing MQTT integration temperature command with default unit");
	
	run_sync([this]() -> boost::cobalt::task<void> {
		co_await broker.start(1883);
	});
	
	Mqtt::MqttIntegration integration(ioc, settings);
	integration.connect_hubs(data_hub, equipment_hub);
	
	run_sync([&integration]() -> boost::cobalt::task<void> {
		co_await integration.start();
	});
	
	broker.clear_published_messages();
	
	// Send temperature command without unit (should default to celsius)
	nlohmann::json temp_command = {
		{"target", "spa"},
		{"temperature", 40.0}
	};
	
	run_sync([this, &temp_command]() -> boost::cobalt::task<void> {
		co_await broker.inject_message("test/command/temperature", temp_command.dump());
		
		boost::asio::steady_timer timer(ioc);
		timer.expires_after(100ms);
		co_await timer.async_wait(boost::asio::use_awaitable);
	});
	
	// Should have published command response with default unit
	auto messages = broker.get_published_messages();
	bool found_response = false;
	
	for (const auto& message : messages)
	{
		if (message.topic == "test/status/command/response")
		{
			nlohmann::json response = nlohmann::json::parse(message.payload);
			BOOST_CHECK_EQUAL(response["command"], "temperature");
			BOOST_CHECK_EQUAL(response["target"], "spa");
			BOOST_CHECK_EQUAL(response["temperature"], 40.0);
			BOOST_CHECK_EQUAL(response["unit"], "celsius");
			found_response = true;
			break;
		}
	}
	
	BOOST_CHECK(found_response);
}

BOOST_FIXTURE_TEST_CASE(Test_MqttIntegration_CustomCommandHandler, MqttIntegrationFixture)
{
	BOOST_TEST_MESSAGE("Testing MQTT integration custom command handler");
	
	run_sync([this]() -> boost::cobalt::task<void> {
		co_await broker.start(1883);
	});
	
	Mqtt::MqttIntegration integration(ioc, settings);
	integration.connect_hubs(data_hub, equipment_hub);
	
	// Add custom command handler
	bool custom_handler_called = false;
	std::string received_topic;
	nlohmann::json received_payload;
	
	auto mqtt_hub = integration.get_mqtt_hub();
	mqtt_hub->register_command_handler("custom",
		[&custom_handler_called, &received_topic, &received_payload](const std::string& topic, const nlohmann::json& payload) -> boost::cobalt::task<void> {
			custom_handler_called = true;
			received_topic = topic;
			received_payload = payload;
			co_return;
		});
	
	run_sync([&integration]() -> boost::cobalt::task<void> {
		co_await integration.start();
	});
	
	// Send custom command
	nlohmann::json custom_command = {{"data", "test_value"}};
	
	run_sync([this, &custom_command]() -> boost::cobalt::task<void> {
		co_await broker.inject_message("test/command/custom", custom_command.dump());
		
		boost::asio::steady_timer timer(ioc);
		timer.expires_after(50ms);
		co_await timer.async_wait(boost::asio::use_awaitable);
	});
	
	BOOST_CHECK(custom_handler_called);
	BOOST_CHECK_EQUAL(received_topic, "test/command/custom");
	BOOST_CHECK_EQUAL(received_payload["data"], "test_value");
}

BOOST_FIXTURE_TEST_CASE(Test_MqttIntegration_ConnectionFailureHandling, MqttIntegrationFixture)
{
	BOOST_TEST_MESSAGE("Testing MQTT integration connection failure handling");
	
	// Don't start the broker - connection should fail
	Mqtt::MqttIntegration integration(ioc, settings);
	integration.connect_hubs(data_hub, equipment_hub);
	
	// Should throw when trying to start without broker
	BOOST_CHECK_THROW({
		run_sync([&integration]() -> boost::cobalt::task<void> {
			co_await integration.start();
		});
	}, std::exception);
}

BOOST_FIXTURE_TEST_CASE(Test_MqttIntegration_DisabledIntegrationDoesNotConnect, MqttIntegrationFixture)
{
	BOOST_TEST_MESSAGE("Testing disabled MQTT integration does not connect");
	
	run_sync([this]() -> boost::cobalt::task<void> {
		co_await broker.start(1883);
	});
	
	settings.is_enabled = false;
	Mqtt::MqttIntegration integration(ioc, settings);
	integration.connect_hubs(data_hub, equipment_hub);
	
	run_sync([&integration]() -> boost::cobalt::task<void> {
		co_await integration.start();
	});
	
	// Should not have connected to broker
	auto connected_clients = broker.get_connected_clients();
	BOOST_CHECK_EQUAL(connected_clients.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()
*/