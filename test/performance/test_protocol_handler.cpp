#include <chrono>
#include <random>
#include <vector>

#include <benchmark/benchmark.h>
#include <boost/circular_buffer.hpp>

#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/generator/jandy_message_generator.h"
#include "jandy/messages/jandy_message_ids.h"
#include "protocol/message_generator_registry.h"

using namespace AqualinkAutomate;

//-----------------------------------------------------------------------------
// MESSAGE FACTORY - CREATION PERFORMANCE
//-----------------------------------------------------------------------------

class MessageFactory_Fixture : public benchmark::Fixture
{
public:
	void SetUp(const ::benchmark::State&) override
	{
	}

	void TearDown(const ::benchmark::State&) override
	{
	}
};

// Benchmark message factory lookup and creation
BENCHMARK_DEFINE_F(MessageFactory_Fixture, CreateMessage_Ack)(benchmark::State& state)
{
	for (auto _ : state)
	{
		auto message = Factory::JandyMessageFactoryT::CreateMessageFromRaw(Messages::JandyMessageIds::Ack);
		benchmark::DoNotOptimize(message);
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(MessageFactory_Fixture, CreateMessage_Ack)->Unit(benchmark::kNanosecond);

BENCHMARK_DEFINE_F(MessageFactory_Fixture, CreateMessage_Probe)(benchmark::State& state)
{
	for (auto _ : state)
	{
		auto message = Factory::JandyMessageFactoryT::CreateMessageFromRaw(Messages::JandyMessageIds::Probe);
		benchmark::DoNotOptimize(message);
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(MessageFactory_Fixture, CreateMessage_Probe)->Unit(benchmark::kNanosecond);

BENCHMARK_DEFINE_F(MessageFactory_Fixture, CreateMessage_Status)(benchmark::State& state)
{
	for (auto _ : state)
	{
		auto message = Factory::JandyMessageFactoryT::CreateMessageFromRaw(Messages::JandyMessageIds::Status);
		benchmark::DoNotOptimize(message);
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(MessageFactory_Fixture, CreateMessage_Status)->Unit(benchmark::kNanosecond);

BENCHMARK_DEFINE_F(MessageFactory_Fixture, CreateMessage_MessageLong)(benchmark::State& state)
{
	for (auto _ : state)
	{
		auto message = Factory::JandyMessageFactoryT::CreateMessageFromRaw(Messages::JandyMessageIds::MessageLong);
		benchmark::DoNotOptimize(message);
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(MessageFactory_Fixture, CreateMessage_MessageLong)->Unit(benchmark::kNanosecond);

//-----------------------------------------------------------------------------
// MESSAGE GENERATOR - PARSING PERFORMANCE
//-----------------------------------------------------------------------------

class MessageGenerator_Fixture : public benchmark::Fixture
{
public:
	void SetUp(const ::benchmark::State& state) override
	{
		buffer.set_capacity(1024);
		BuildTestMessage(state.range(0));
	}

	void TearDown(const ::benchmark::State&) override
	{
		buffer.clear();
	}

	void BuildTestMessage(int message_type)
	{
		buffer.clear();

		switch (message_type)
		{
		case 0: // ACK message (minimal)
			BuildAckMessage();
			break;
		case 1: // Probe message
			BuildProbeMessage();
			break;
		case 2: // Status message (medium complexity)
			BuildStatusMessage();
			break;
		case 3: // Message Long (large payload)
			BuildMessageLong();
			break;
		default:
			BuildAckMessage();
			break;
		}
	}

	void BuildAckMessage()
	{
		// Minimal ACK: DLE STX DEST CMD CHKSUM DLE ETX
		buffer.push_back(0x10); // DLE
		buffer.push_back(0x02); // STX
		buffer.push_back(0x00); // Destination (Master)
		buffer.push_back(0x01); // Command (Ack)
		buffer.push_back(0x00); // Data byte
		buffer.push_back(0x13); // Checksum
		buffer.push_back(0x10); // DLE
		buffer.push_back(0x03); // ETX
	}

	void BuildProbeMessage()
	{
		buffer.push_back(0x10); // DLE
		buffer.push_back(0x02); // STX
		buffer.push_back(0x48); // Destination
		buffer.push_back(0x00); // Command (Probe)
		buffer.push_back(0x5A); // Checksum
		buffer.push_back(0x10); // DLE
		buffer.push_back(0x03); // ETX
	}

	void BuildStatusMessage()
	{
		buffer.push_back(0x10); // DLE
		buffer.push_back(0x02); // STX
		buffer.push_back(0x00); // Destination
		buffer.push_back(0x02); // Command (Status)
		// 16 bytes of status data
		for (int i = 0; i < 16; ++i)
		{
			buffer.push_back(static_cast<uint8_t>(i));
		}
		buffer.push_back(0x00); // Checksum placeholder
		buffer.push_back(0x10); // DLE
		buffer.push_back(0x03); // ETX

		// Fix checksum
		uint8_t checksum = 0;
		for (size_t i = 2; i < buffer.size() - 3; ++i)
		{
			checksum += buffer[i];
		}
		buffer[buffer.size() - 3] = checksum;
	}

	void BuildMessageLong()
	{
		buffer.push_back(0x10); // DLE
		buffer.push_back(0x02); // STX
		buffer.push_back(0x00); // Destination
		buffer.push_back(0x04); // Command (MessageLong)
		buffer.push_back(0x00); // Line ID
		// 128 bytes of text data
		for (int i = 0; i < 128; ++i)
		{
			char c = 'A' + (i % 26);
			buffer.push_back(static_cast<uint8_t>(c));
		}
		buffer.push_back(0x00); // Checksum placeholder
		buffer.push_back(0x10); // DLE
		buffer.push_back(0x03); // ETX

		// Fix checksum
		uint8_t checksum = 0;
		for (size_t i = 2; i < buffer.size() - 3; ++i)
		{
			checksum += buffer[i];
		}
		buffer[buffer.size() - 3] = checksum;
	}

public:
	boost::circular_buffer<uint8_t> buffer;
};

// Benchmark parsing ACK message (minimal)
BENCHMARK_DEFINE_F(MessageGenerator_Fixture, ParseMessage_Ack)(benchmark::State& state)
{
	for (auto _ : state)
	{
		// Restore buffer state for next iteration
		state.PauseTiming();
		BuildAckMessage();
		state.ResumeTiming();

		auto result = Generators::GenerateMessageFromRawData(buffer);
		benchmark::DoNotOptimize(result);
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations());
	state.SetBytesProcessed(state.iterations() * 8); // ACK is 8 bytes
}
BENCHMARK_REGISTER_F(MessageGenerator_Fixture, ParseMessage_Ack)->Arg(0)->Unit(benchmark::kNanosecond);

// Benchmark parsing Status message (medium)
BENCHMARK_DEFINE_F(MessageGenerator_Fixture, ParseMessage_Status)(benchmark::State& state)
{
	for (auto _ : state)
	{
		state.PauseTiming();
		BuildStatusMessage();
		state.ResumeTiming();

		auto result = Generators::GenerateMessageFromRawData(buffer);
		benchmark::DoNotOptimize(result);
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations());
	state.SetBytesProcessed(state.iterations() * 25); // Status is ~25 bytes
}
BENCHMARK_REGISTER_F(MessageGenerator_Fixture, ParseMessage_Status)->Arg(2)->Unit(benchmark::kNanosecond);

// Benchmark parsing MessageLong (large payload)
BENCHMARK_DEFINE_F(MessageGenerator_Fixture, ParseMessage_Long)(benchmark::State& state)
{
	for (auto _ : state)
	{
		state.PauseTiming();
		BuildMessageLong();
		state.ResumeTiming();

		auto result = Generators::GenerateMessageFromRawData(buffer);
		benchmark::DoNotOptimize(result);
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations());
	state.SetBytesProcessed(state.iterations() * 138); // MessageLong is ~138 bytes
}
BENCHMARK_REGISTER_F(MessageGenerator_Fixture, ParseMessage_Long)->Arg(3)->Unit(benchmark::kMicrosecond);

//-----------------------------------------------------------------------------
// CIRCULAR BUFFER OPERATIONS
//-----------------------------------------------------------------------------

class CircularBuffer_Fixture : public benchmark::Fixture
{
public:
	void SetUp(const ::benchmark::State& state) override
	{
		buffer.set_capacity(static_cast<size_t>(state.range(0)));

		// Pre-fill with random data
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int> dist(0, 255);

		test_data.resize(256);
		for (auto& byte : test_data)
		{
			byte = static_cast<uint8_t>(dist(gen));
		}
	}

	void TearDown(const ::benchmark::State&) override
	{
		buffer.clear();
	}

public:
	boost::circular_buffer<uint8_t> buffer;
	std::vector<uint8_t> test_data;
};

// Benchmark push_back performance at different buffer sizes
BENCHMARK_DEFINE_F(CircularBuffer_Fixture, PushBack)(benchmark::State& state)
{
	size_t index = 0;
	for (auto _ : state)
	{
		buffer.push_back(test_data[index % test_data.size()]);
		index++;
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(CircularBuffer_Fixture, PushBack)
	->Arg(256)
	->Arg(512)
	->Arg(1024)
	->Arg(4096)
	->Unit(benchmark::kNanosecond);

// Benchmark bulk push (simulating serial read)
BENCHMARK_DEFINE_F(CircularBuffer_Fixture, BulkPush)(benchmark::State& state)
{
	size_t chunk_size = state.range(1);

	for (auto _ : state)
	{
		for (size_t i = 0; i < chunk_size; ++i)
		{
			buffer.push_back(test_data[i % test_data.size()]);
		}
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations() * chunk_size);
}
BENCHMARK_REGISTER_F(CircularBuffer_Fixture, BulkPush)
	->Args({1024, 32})   // Buffer size, chunk size
	->Args({1024, 64})
	->Args({1024, 128})
	->Args({1024, 256})
	->Unit(benchmark::kNanosecond);

//-----------------------------------------------------------------------------
// MESSAGE GENERATOR REGISTRY
//-----------------------------------------------------------------------------

class GeneratorRegistry_Fixture : public benchmark::Fixture
{
public:
	void SetUp(const ::benchmark::State&) override
	{
		buffer.set_capacity(1024);

		// Build a valid ACK message
		buffer.push_back(0x10); // DLE
		buffer.push_back(0x02); // STX
		buffer.push_back(0x00); // Destination
		buffer.push_back(0x01); // Command (Ack)
		buffer.push_back(0x00); // Data
		buffer.push_back(0x13); // Checksum
		buffer.push_back(0x10); // DLE
		buffer.push_back(0x03); // ETX
	}

	void TearDown(const ::benchmark::State&) override
	{
		buffer.clear();
	}

public:
	boost::circular_buffer<uint8_t> buffer;
};

// Benchmark the message generator registry lookup and dispatch
BENCHMARK_DEFINE_F(GeneratorRegistry_Fixture, RegistryGenerateMessage)(benchmark::State& state)
{
	for (auto _ : state)
	{
		state.PauseTiming();
		buffer.clear();
		buffer.push_back(0x10);
		buffer.push_back(0x02);
		buffer.push_back(0x00);
		buffer.push_back(0x01);
		buffer.push_back(0x00);
		buffer.push_back(0x13);
		buffer.push_back(0x10);
		buffer.push_back(0x03);
		state.ResumeTiming();

		auto result = Protocol::MessageGeneratorRegistry::Instance().GenerateMessage(buffer);
		benchmark::DoNotOptimize(result);
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(GeneratorRegistry_Fixture, RegistryGenerateMessage)->Unit(benchmark::kNanosecond);
