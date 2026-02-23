#include <chrono>
#include <future>
#include <memory>
#include <vector>

#include <boost/asio.hpp>
#include <benchmark/benchmark.h>
#include <nlohmann/json.hpp>

#include "mqtt/mqtt_client.h"
#include "mqtt/mqtt_hub.h"
#include "mqtt/mqtt_integration.h"
#include "kernel/data_hub.h"
#include "kernel/equipment_hub.h"
#include "options/options_mqtt_options.h"
//#include "mocks/mock_mqtt_broker.h"

using namespace AqualinkAutomate;
//using namespace AqualinkAutomate::Test::Mocks;
using namespace std::chrono_literals;
/*
// Helper class to manage async operations in benchmarks
class BenchmarkAsyncHelper
{
public:
	BenchmarkAsyncHelper() = default;
	~BenchmarkAsyncHelper() = default;

	template<typename F>
	auto run(F&& f) -> decltype(f().result())
	{
		auto future = std::async(std::launch::async, [&]() {
			return boost::asio::co_spawn(ioc, std::forward<F>(f), boost::asio::use_future);
		});
		
		ioc.restart();
		ioc.run();
		
		return future.get().get();
	}

	boost::asio::io_context ioc;
};

// Benchmark MQTT client connection establishment
static void BM_MqttClient_ConnectionEstablishment(benchmark::State& state)
{
	BenchmarkAsyncHelper helper;
	MockMqttBroker broker(helper.ioc);
	
	Options::Mqtt::Settings settings;
	settings.broker_host = "localhost";
	settings.broker_port = 1883;
	settings.auto_reconnect = false;
	
	helper.run([&broker]() -> boost::cobalt::task<void> {
		co_await broker.start(1883);
	});

	for ([[maybe_unused]] auto _ : state)
	{
		state.PauseTiming();
		
		auto client = std::make_unique<Mqtt::MqttClient>(helper.ioc, settings);
		
		state.ResumeTiming();
		
		auto start = std::chrono::high_resolution_clock::now();
		
		helper.run([&client]() -> boost::cobalt::task<void> {
			co_await client->connect();
		});
		
		auto end = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		
		state.SetIterationTime(elapsed.count() / 1000000.0);
		
		state.PauseTiming();
		
		helper.run([&client]() -> boost::cobalt::task<void> {
			co_await client->disconnect();
		});
		
		state.ResumeTiming();
	}
	
	helper.run([&broker]() -> boost::cobalt::task<void> {
		co_await broker.stop();
	});
}
BENCHMARK(BM_MqttClient_ConnectionEstablishment)->UseManualTime()->Unit(benchmark::kMillisecond);

// Benchmark MQTT message publishing throughput
static void BM_MqttClient_PublishThroughput(benchmark::State& state)
{
	BenchmarkAsyncHelper helper;
	MockMqttBroker broker(helper.ioc);
	
	Options::Mqtt::Settings settings;
	settings.broker_host = "localhost";
	settings.broker_port = 1883;
	settings.qos = static_cast<uint8_t>(state.range(0));
	
	helper.run([&broker]() -> boost::cobalt::task<void> {
		co_await broker.start(1883);
	});
	
	Mqtt::MqttClient client(helper.ioc, settings);
	
	helper.run([&client]() -> boost::cobalt::task<void> {
		co_await client.connect();
	});
	
	const std::string topic = "test/performance/topic";
	const std::string payload = R"({"timestamp": 1640995200, "temperature": 25.5, "status": "online"})";
	
	int64_t message_count = 0;
	
	for ([[maybe_unused]] auto _ : state)
	{
		auto start = std::chrono::high_resolution_clock::now();
		
		helper.run([&client, &topic, &payload]() -> boost::cobalt::task<void> {
			co_await client.publish(topic, payload);
		});
		
		auto end = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		
		state.SetIterationTime(elapsed.count() / 1000000.0);
		message_count++;
	}
	
	state.SetItemsProcessed(message_count);
	state.SetBytesProcessed(message_count * (topic.size() + payload.size()));
	
	helper.run([&client]() -> boost::cobalt::task<void> {
		co_await client.disconnect();
	});
	
	helper.run([&broker]() -> boost::cobalt::task<void> {
		co_await broker.stop();
	});
}
BENCHMARK(BM_MqttClient_PublishThroughput)
	->Args({0})  // QoS 0
	->Args({1})  // QoS 1
	->Args({2})  // QoS 2
	->UseManualTime()
	->Unit(benchmark::kMicrosecond);

// Benchmark MQTT message publishing with varying payload sizes
static void BM_MqttClient_PublishPayloadSize(benchmark::State& state)
{
	BenchmarkAsyncHelper helper;
	MockMqttBroker broker(helper.ioc);
	
	Options::Mqtt::Settings settings;
	settings.broker_host = "localhost";
	settings.broker_port = 1883;
	settings.qos = 1;
	
	helper.run([&broker]() -> boost::cobalt::task<void> {
		co_await broker.start(1883);
	});
	
	Mqtt::MqttClient client(helper.ioc, settings);
	
	helper.run([&client]() -> boost::cobalt::task<void> {
		co_await client.connect();
	});
	
	const std::string topic = "test/performance/payload";
	const int64_t payload_size = state.range(0);
	const std::string payload(payload_size, 'A');
	
	int64_t message_count = 0;
	
	for ([[maybe_unused]] auto _ : state)
	{
		auto start = std::chrono::high_resolution_clock::now();
		
		helper.run([&client, &topic, &payload]() -> boost::cobalt::task<void> {
			co_await client.publish(topic, payload);
		});
		
		auto end = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		
		state.SetIterationTime(elapsed.count() / 1000000.0);
		message_count++;
	}
	
	state.SetItemsProcessed(message_count);
	state.SetBytesProcessed(message_count * (topic.size() + payload_size));
	
	helper.run([&client]() -> boost::cobalt::task<void> {
		co_await client.disconnect();
	});
	
	helper.run([&broker]() -> boost::cobalt::task<void> {
		co_await broker.stop();
	});
}
BENCHMARK(BM_MqttClient_PublishPayloadSize)
	->Args({64})     // 64 bytes
	->Args({256})    // 256 bytes
	->Args({1024})   // 1 KB
	->Args({4096})   // 4 KB
	->Args({16384})  // 16 KB
	->UseManualTime()
	->Unit(benchmark::kMicrosecond);

// Benchmark MQTT subscription performance
static void BM_MqttClient_SubscriptionPerformance(benchmark::State& state)
{
	BenchmarkAsyncHelper helper;
	MockMqttBroker broker(helper.ioc);
	
	Options::Mqtt::Settings settings;
	settings.broker_host = "localhost";
	settings.broker_port = 1883;
	
	helper.run([&broker]() -> boost::cobalt::task<void> {
		co_await broker.start(1883);
	});
	
	Mqtt::MqttClient client(helper.ioc, settings);
	
	helper.run([&client]() -> boost::cobalt::task<void> {
		co_await client.connect();
	});
	
	int64_t subscription_count = 0;
	
	for ([[maybe_unused]] auto _ : state)
	{
		std::string topic_filter = "test/perf/topic" + std::to_string(subscription_count);
		
		auto start = std::chrono::high_resolution_clock::now();
		
		helper.run([&client, &topic_filter]() -> boost::cobalt::task<void> {
			co_await client.subscribe(topic_filter);
		});
		
		auto end = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		
		state.SetIterationTime(elapsed.count() / 1000000.0);
		subscription_count++;
	}
	
	state.SetItemsProcessed(subscription_count);
	
	helper.run([&client]() -> boost::cobalt::task<void> {
		co_await client.disconnect();
	});
	
	helper.run([&broker]() -> boost::cobalt::task<void> {
		co_await broker.stop();
	});
}
BENCHMARK(BM_MqttClient_SubscriptionPerformance)->UseManualTime()->Unit(benchmark::kMicrosecond);

// Benchmark MQTT hub status publishing
static void BM_MqttHub_StatusPublishing(benchmark::State& state)
{
	BenchmarkAsyncHelper helper;
	MockMqttBroker broker(helper.ioc);
	
	Options::Mqtt::Settings settings;
	settings.broker_host = "localhost";
	settings.broker_port = 1883;
	settings.topic_prefix = "bench";
	
	helper.run([&broker]() -> boost::cobalt::task<void> {
		co_await broker.start(1883);
	});
	
	Mqtt::MqttHub hub(helper.ioc, settings);
	auto data_hub = std::make_shared<Kernel::DataHub>();
	auto equipment_hub = std::make_shared<Kernel::EquipmentHub>();
	
	hub.connect_data_hub(data_hub);
	hub.connect_equipment_hub(equipment_hub);
	
	helper.run([&hub]() -> boost::cobalt::task<void> {
		co_await hub.start();
	});
	
	int64_t publish_count = 0;
	
	for ([[maybe_unused]] auto _ : state)
	{
		broker.clear_published_messages();
		
		auto start = std::chrono::high_resolution_clock::now();
		
		helper.run([&hub]() -> boost::cobalt::task<void> {
			co_await hub.publish_status();
		});
		
		auto end = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		
		state.SetIterationTime(elapsed.count() / 1000000.0);
		publish_count++;
		
		// Calculate total bytes published
		auto messages = broker.get_published_messages();
		int64_t total_bytes = 0;
		for (const auto& msg : messages)
		{
			total_bytes += msg.topic.size() + msg.payload.size();
		}
		state.counters["MessagesPerPublish"] = static_cast<double>(messages.size());
		state.counters["BytesPerPublish"] = static_cast<double>(total_bytes);
	}
	
	state.SetItemsProcessed(publish_count);
	
	helper.run([&hub]() -> boost::cobalt::task<void> {
		co_await hub.stop();
	});
	
	helper.run([&broker]() -> boost::cobalt::task<void> {
		co_await broker.stop();
	});
}
BENCHMARK(BM_MqttHub_StatusPublishing)->UseManualTime()->Unit(benchmark::kMicrosecond);

// Benchmark MQTT hub command processing
static void BM_MqttHub_CommandProcessing(benchmark::State& state)
{
	BenchmarkAsyncHelper helper;
	MockMqttBroker broker(helper.ioc);
	
	Options::Mqtt::Settings settings;
	settings.broker_host = "localhost";
	settings.broker_port = 1883;
	settings.topic_prefix = "bench";
	
	helper.run([&broker]() -> boost::cobalt::task<void> {
		co_await broker.start(1883);
	});
	
	Mqtt::MqttHub hub(helper.ioc, settings);
	
	// Register a simple command handler
	int command_count = 0;
	hub.register_command_handler("test", 
		[&command_count](const std::string& topic, const nlohmann::json& payload) -> boost::cobalt::task<void> {
			command_count++;
			co_return;
		});
	
	helper.run([&hub]() -> boost::cobalt::task<void> {
		co_await hub.start();
	});
	
	const nlohmann::json command_payload = {{"action", "test"}, {"value", 42}};
	const std::string command_json = command_payload.dump();
	
	int64_t message_count = 0;
	
	for ([[maybe_unused]] auto _ : state)
	{
		auto start = std::chrono::high_resolution_clock::now();
		
		helper.run([&broker, &command_json]() -> boost::cobalt::task<void> {
			co_await broker.inject_message("bench/command/test", command_json);
			
			// Give time for command processing
			boost::asio::steady_timer timer(broker.m_ioc);
			timer.expires_after(1ms);
			co_await timer.async_wait(boost::asio::use_awaitable);
		});
		
		auto end = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		
		state.SetIterationTime(elapsed.count() / 1000000.0);
		message_count++;
	}
	
	state.SetItemsProcessed(message_count);
	state.SetBytesProcessed(message_count * command_json.size());
	state.counters["CommandsProcessed"] = static_cast<double>(command_count);
	
	helper.run([&hub]() -> boost::cobalt::task<void> {
		co_await hub.stop();
	});
	
	helper.run([&broker]() -> boost::cobalt::task<void> {
		co_await broker.stop();
	});
}
BENCHMARK(BM_MqttHub_CommandProcessing)->UseManualTime()->Unit(benchmark::kMicrosecond);

// Benchmark JSON serialization for status messages
static void BM_MqttHub_JsonSerialization(benchmark::State& state)
{
	// Create a data hub with some test data
	auto data_hub = std::make_shared<Kernel::DataHub>();
	data_hub->AirTemp(Kernel::Temperature::ConvertToTemperatureInCelsius(25.5f));
	data_hub->PoolTemp(Kernel::Temperature::ConvertToTemperatureInCelsius(28.0f));
	data_hub->SpaTemp(Kernel::Temperature::ConvertToTemperatureInCelsius(40.0f));
	
	int64_t serialization_count = 0;
	int64_t total_bytes = 0;
	
	for ([[maybe_unused]] auto _ : state)
	{
		auto start = std::chrono::high_resolution_clock::now();
		
		// Simulate the JSON serialization done by MqttHub
		nlohmann::json status = {
			{"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
				std::chrono::system_clock::now().time_since_epoch()).count()},
			{"online", true},
			{"temperatures", {
				{"air", {
					{"celsius", data_hub->AirTemp().ValueInCelsius()},
					{"fahrenheit", data_hub->AirTemp().ValueInFahrenheit()}
				}},
				{"pool", {
					{"celsius", data_hub->PoolTemp().ValueInCelsius()},
					{"fahrenheit", data_hub->PoolTemp().ValueInFahrenheit()}
				}},
				{"spa", {
					{"celsius", data_hub->SpaTemp().ValueInCelsius()},
					{"fahrenheit", data_hub->SpaTemp().ValueInFahrenheit()}
				}}
			}},
			{"chemistry", {
				{"orp_mv", data_hub->ORP().ValueInMillivolts()},
				{"ph", data_hub->pH().Value()},
				{"salt_ppm", data_hub->SaltLevel().value()}
			}}
		};
		
		std::string json_string = status.dump();
		
		auto end = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
		
		state.SetIterationTime(elapsed.count() / 1000000000.0);
		serialization_count++;
		total_bytes += json_string.size();
	}
	
	state.SetItemsProcessed(serialization_count);
	state.SetBytesProcessed(total_bytes);
}
BENCHMARK(BM_MqttHub_JsonSerialization)->UseManualTime()->Unit(benchmark::kNanosecond);

// Benchmark MQTT integration end-to-end performance
static void BM_MqttIntegration_EndToEnd(benchmark::State& state)
{
	BenchmarkAsyncHelper helper;
	MockMqttBroker broker(helper.ioc);
	
	Options::Mqtt::Settings settings;
	settings.is_enabled = true;
	settings.broker_host = "localhost";
	settings.broker_port = 1883;
	settings.topic_prefix = "bench";
	
	helper.run([&broker]() -> boost::cobalt::task<void> {
		co_await broker.start(1883);
	});
	
	auto data_hub = std::make_shared<Kernel::DataHub>();
	auto equipment_hub = std::make_shared<Kernel::EquipmentHub>();
	
	int64_t operation_count = 0;
	
	for ([[maybe_unused]] auto _ : state)
	{
		state.PauseTiming();
		
		Mqtt::MqttIntegration integration(helper.ioc, settings);
		integration.connect_hubs(data_hub, equipment_hub);
		
		state.ResumeTiming();
		
		auto start = std::chrono::high_resolution_clock::now();
		
		helper.run([&integration]() -> boost::cobalt::task<void> {
			co_await integration.start();
		});
		
		// Send a status command
		helper.run([&broker]() -> boost::cobalt::task<void> {
			co_await broker.inject_message("bench/command/status", "{}");
			
			// Give time for processing
			boost::asio::steady_timer timer(broker.m_ioc);
			timer.expires_after(10ms);
			co_await timer.async_wait(boost::asio::use_awaitable);
		});
		
		helper.run([&integration]() -> boost::cobalt::task<void> {
			co_await integration.stop();
		});
		
		auto end = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		
		state.SetIterationTime(elapsed.count() / 1000.0);
		operation_count++;
		
		state.PauseTiming();
		broker.clear_published_messages();
		state.ResumeTiming();
	}
	
	state.SetItemsProcessed(operation_count);
	
	helper.run([&broker]() -> boost::cobalt::task<void> {
		co_await broker.stop();
	});
}
BENCHMARK(BM_MqttIntegration_EndToEnd)->UseManualTime()->Unit(benchmark::kMillisecond);

// Memory usage benchmark
static void BM_MqttClient_MemoryUsage(benchmark::State& state)
{
	BenchmarkAsyncHelper helper;
	MockMqttBroker broker(helper.ioc);
	
	Options::Mqtt::Settings settings;
	settings.broker_host = "localhost";
	settings.broker_port = 1883;
	
	helper.run([&broker]() -> boost::cobalt::task<void> {
		co_await broker.start(1883);
	});
	
	for ([[maybe_unused]] auto _ : state)
	{
		state.PauseTiming();
		
		// Create many clients to test memory scaling
		std::vector<std::unique_ptr<Mqtt::MqttClient>> clients;
		const int client_count = state.range(0);
		
		state.ResumeTiming();
		
		// Create clients
		for (int i = 0; i < client_count; ++i)
		{
			auto client = std::make_unique<Mqtt::MqttClient>(helper.ioc, settings);
			clients.push_back(std::move(client));
		}
		
		// Connect all clients
		for (auto& client : clients)
		{
			helper.run([&client]() -> boost::cobalt::task<void> {
				co_await client->connect();
			});
		}
		
		// Disconnect all clients
		for (auto& client : clients)
		{
			helper.run([&client]() -> boost::cobalt::task<void> {
				co_await client->disconnect();
			});
		}
		
		state.PauseTiming();
		clients.clear();
		state.ResumeTiming();
	}
	
	helper.run([&broker]() -> boost::cobalt::task<void> {
		co_await broker.stop();
	});
}
BENCHMARK(BM_MqttClient_MemoryUsage)
	->Args({1})
	->Args({10})
	->Args({50})
	->Args({100})
	->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
*/