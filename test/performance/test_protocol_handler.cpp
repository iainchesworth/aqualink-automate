#include <chrono>
#include <random>
#include <vector>

#include <benchmark/benchmark.h>
#include <boost/circular_buffer.hpp>

#include "errors/protocol_errors.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/generator/jandy_message_generator.h"
#include "jandy/messages/jandy_message_ids.h"
#include "kernel/statistics_hub.h"
#include "protocol/message_generator_registry.h"
#include "protocol/protocol_handler_read.h"

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
	for ([[maybe_unused]] auto _ : state)
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
	for ([[maybe_unused]] auto _ : state)
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
	for ([[maybe_unused]] auto _ : state)
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
	for ([[maybe_unused]] auto _ : state)
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
		BuildTestMessage(static_cast<int>(state.range(0)));
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
			char c = static_cast<char>('A' + (i % 26));
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
	for ([[maybe_unused]] auto _ : state)
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
	for ([[maybe_unused]] auto _ : state)
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
	for ([[maybe_unused]] auto _ : state)
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
	for ([[maybe_unused]] auto _ : state)
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

	for ([[maybe_unused]] auto _ : state)
	{
		for (size_t i = 0; i < chunk_size; ++i)
		{
			buffer.push_back(test_data[i % test_data.size()]);
		}
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * chunk_size));
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
	for ([[maybe_unused]] auto _ : state)
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

//-----------------------------------------------------------------------------
// PROCESS MESSAGES LOOP - MULTI-MESSAGE THROUGHPUT
//-----------------------------------------------------------------------------
// These benchmarks simulate what ProtocolTask::ProcessMessages does: parsing
// multiple messages from a circular buffer in a tight loop until the buffer
// is drained.  Tracking these over time reveals regressions in per-message
// processing cost and loop overhead.

namespace
{
	// Helper: compute Jandy checksum for bytes in [start, end) and append
	// checksum + DLE + ETX.
	void AppendChecksumAndTrailer(std::vector<uint8_t>& packet, std::size_t header_start)
	{
		uint8_t checksum = 0;
		for (std::size_t i = header_start + 2; i < packet.size(); ++i)  // skip DLE, STX
		{
			checksum += packet[i];
		}
		packet.push_back(checksum);
		packet.push_back(0x10); // DLE
		packet.push_back(0x03); // ETX
	}

	// Build a valid ACK packet (8 bytes) and append to vec.
	void AppendAckPacket(std::vector<uint8_t>& vec)
	{
		auto start = vec.size();
		vec.push_back(0x10); // DLE
		vec.push_back(0x02); // STX
		vec.push_back(0x00); // Destination
		vec.push_back(0x01); // Command (Ack)
		vec.push_back(0x00); // Data
		AppendChecksumAndTrailer(vec, start);
	}

	// Build a valid Probe packet (7 bytes) and append to vec.
	void AppendProbePacket(std::vector<uint8_t>& vec)
	{
		auto start = vec.size();
		vec.push_back(0x10); // DLE
		vec.push_back(0x02); // STX
		vec.push_back(0x48); // Destination
		vec.push_back(0x00); // Command (Probe)
		AppendChecksumAndTrailer(vec, start);
	}

	// Build a valid Status packet (~21 bytes) and append to vec.
	void AppendStatusPacket(std::vector<uint8_t>& vec)
	{
		auto start = vec.size();
		vec.push_back(0x10); // DLE
		vec.push_back(0x02); // STX
		vec.push_back(0x00); // Destination
		vec.push_back(0x02); // Command (Status)
		for (int i = 0; i < 16; ++i)
		{
			vec.push_back(static_cast<uint8_t>(i));
		}
		AppendChecksumAndTrailer(vec, start);
	}

	// Build a packet with an invalid checksum (triggers ChecksumFailure).
	void AppendBadChecksumPacket(std::vector<uint8_t>& vec)
	{
		vec.push_back(0x10); // DLE
		vec.push_back(0x02); // STX
		vec.push_back(0x00); // Destination
		vec.push_back(0x01); // Command (Ack)
		vec.push_back(0x00); // Data
		vec.push_back(0xFF); // Wrong checksum
		vec.push_back(0x10); // DLE
		vec.push_back(0x03); // ETX
	}

	// Build an overlapping-packets scenario: packet without end followed by another start.
	void AppendOverlappingPackets(std::vector<uint8_t>& vec)
	{
		// First packet: has start but no end before second start appears
		vec.push_back(0x10); // DLE
		vec.push_back(0x02); // STX
		vec.push_back(0x40); // Destination
		vec.push_back(0x02); // Command
		vec.push_back(0x00);
		vec.push_back(0x00);
		vec.push_back(0x00);
		vec.push_back(0x40);
		vec.push_back(0x04);
		vec.push_back(0x00);
		vec.push_back(0x00);
		vec.push_back(0x00); // No DLE ETX here — incomplete

		// Second packet start immediately follows (triggers OverlappingPackets)
		// followed by a valid ACK so the recovery can parse it
		AppendAckPacket(vec);
	}

	// Fill a circular buffer from a vector.
	void FillBuffer(boost::circular_buffer<uint8_t>& buf, const std::vector<uint8_t>& data)
	{
		buf.clear();
		for (auto b : data)
		{
			buf.push_back(b);
		}
	}

	// Simulate the ProcessMessages loop: parse until buffer is drained or
	// we get WaitingForMoreData.
	std::size_t RunProcessMessagesLoop(boost::circular_buffer<uint8_t>& buffer)
	{
		std::size_t messages_parsed = 0;
		while (!buffer.empty())
		{
			auto result = Protocol::MessageGeneratorRegistry::Instance().GenerateMessage(buffer);
			if (result)
			{
				++messages_parsed;
			}
			else
			{
				auto ev = result.error().value();
				if (ev == ErrorCodes::Protocol_ErrorCodes::DataAvailableToProcess
					|| ev == ErrorCodes::Protocol_ErrorCodes::ChecksumFailure
					|| ev == ErrorCodes::Protocol_ErrorCodes::OverlappingPackets)
				{
					continue;
				}
				break;
			}
		}
		return messages_parsed;
	}
}

class ProcessMessages_Fixture : public benchmark::Fixture
{
public:
	void SetUp(const ::benchmark::State&) override
	{
		buffer.set_capacity(4096);
	}

	void TearDown(const ::benchmark::State&) override
	{
		buffer.clear();
	}

public:
	boost::circular_buffer<uint8_t> buffer;
};

// Benchmark: consecutive valid ACK messages (best-case throughput).
// Parameterized by message count: 1, 5, 10, 20.
BENCHMARK_DEFINE_F(ProcessMessages_Fixture, ConsecutiveValid_Ack)(benchmark::State& state)
{
	const auto count = static_cast<std::size_t>(state.range(0));

	std::vector<uint8_t> stream;
	for (std::size_t i = 0; i < count; ++i)
	{
		AppendAckPacket(stream);
	}

	for ([[maybe_unused]] auto _ : state)
	{
		state.PauseTiming();
		FillBuffer(buffer, stream);
		state.ResumeTiming();

		auto parsed = RunProcessMessagesLoop(buffer);
		benchmark::DoNotOptimize(parsed);
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(count));
	state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(stream.size()));
}
BENCHMARK_REGISTER_F(ProcessMessages_Fixture, ConsecutiveValid_Ack)
	->Arg(1)->Arg(5)->Arg(10)->Arg(20)
	->Unit(benchmark::kMicrosecond);

// Benchmark: consecutive valid Status messages (medium payload).
BENCHMARK_DEFINE_F(ProcessMessages_Fixture, ConsecutiveValid_Status)(benchmark::State& state)
{
	const auto count = static_cast<std::size_t>(state.range(0));

	std::vector<uint8_t> stream;
	for (std::size_t i = 0; i < count; ++i)
	{
		AppendStatusPacket(stream);
	}

	for ([[maybe_unused]] auto _ : state)
	{
		state.PauseTiming();
		FillBuffer(buffer, stream);
		state.ResumeTiming();

		auto parsed = RunProcessMessagesLoop(buffer);
		benchmark::DoNotOptimize(parsed);
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(count));
	state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(stream.size()));
}
BENCHMARK_REGISTER_F(ProcessMessages_Fixture, ConsecutiveValid_Status)
	->Arg(1)->Arg(5)->Arg(10)->Arg(20)
	->Unit(benchmark::kMicrosecond);

//-----------------------------------------------------------------------------
// ERROR RECOVERY PATHS
//-----------------------------------------------------------------------------

// Benchmark: checksum failure recovery cost.
// Buffer contains N bad-checksum packets followed by 1 valid ACK.
// Measures how quickly the generator discards corrupt packets and recovers.
BENCHMARK_DEFINE_F(ProcessMessages_Fixture, ChecksumRecovery)(benchmark::State& state)
{
	const auto bad_count = static_cast<std::size_t>(state.range(0));

	std::vector<uint8_t> stream;
	for (std::size_t i = 0; i < bad_count; ++i)
	{
		AppendBadChecksumPacket(stream);
	}
	AppendAckPacket(stream);

	for ([[maybe_unused]] auto _ : state)
	{
		state.PauseTiming();
		FillBuffer(buffer, stream);
		state.ResumeTiming();

		auto parsed = RunProcessMessagesLoop(buffer);
		benchmark::DoNotOptimize(parsed);
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations());
	state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(stream.size()));
}
BENCHMARK_REGISTER_F(ProcessMessages_Fixture, ChecksumRecovery)
	->Arg(1)->Arg(3)->Arg(5)->Arg(10)
	->Unit(benchmark::kMicrosecond);

// Benchmark: overlapping packet recovery cost.
// Buffer contains an overlapping-packet scenario followed by the valid
// packet that the generator should recover and parse.
BENCHMARK_DEFINE_F(ProcessMessages_Fixture, OverlappingRecovery)(benchmark::State& state)
{
	std::vector<uint8_t> stream;
	AppendOverlappingPackets(stream);

	for ([[maybe_unused]] auto _ : state)
	{
		state.PauseTiming();
		FillBuffer(buffer, stream);
		state.ResumeTiming();

		auto parsed = RunProcessMessagesLoop(buffer);
		benchmark::DoNotOptimize(parsed);
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations());
	state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(stream.size()));
}
BENCHMARK_REGISTER_F(ProcessMessages_Fixture, OverlappingRecovery)
	->Unit(benchmark::kMicrosecond);

//-----------------------------------------------------------------------------
// MIXED WORKLOAD (Realistic Serial Stream)
//-----------------------------------------------------------------------------
// Simulates a realistic bus capture: mostly valid packets with occasional
// corruption.  Pattern per batch: 8 valid ACK/Probe, 1 bad checksum, 1 valid Status.

BENCHMARK_DEFINE_F(ProcessMessages_Fixture, MixedWorkload)(benchmark::State& state)
{
	const auto batches = static_cast<std::size_t>(state.range(0));

	std::vector<uint8_t> stream;
	for (std::size_t b = 0; b < batches; ++b)
	{
		// 4 Probes + 4 ACKs (typical poll cycle)
		for (int i = 0; i < 4; ++i)
		{
			AppendProbePacket(stream);
			AppendAckPacket(stream);
		}
		// 1 corrupt packet
		AppendBadChecksumPacket(stream);
		// 1 Status message
		AppendStatusPacket(stream);
	}

	for ([[maybe_unused]] auto _ : state)
	{
		state.PauseTiming();
		FillBuffer(buffer, stream);
		state.ResumeTiming();

		auto parsed = RunProcessMessagesLoop(buffer);
		benchmark::DoNotOptimize(parsed);
		benchmark::ClobberMemory();
	}

	// 9 valid messages per batch (4 probe + 4 ack + 1 status)
	state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(batches * 9));
	state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(stream.size()));
}
BENCHMARK_REGISTER_F(ProcessMessages_Fixture, MixedWorkload)
	->Arg(1)->Arg(5)->Arg(10)
	->Unit(benchmark::kMicrosecond);

//-----------------------------------------------------------------------------
// ERROR HANDLER OVERHEAD
//-----------------------------------------------------------------------------
// Measures the cost of the error classification + statistics counter update
// path that fires on every protocol error.

class ErrorHandler_Fixture : public benchmark::Fixture
{
public:
	void SetUp(const ::benchmark::State&) override
	{
		stats_hub = std::make_shared<Kernel::StatisticsHub>();
	}

	void TearDown(const ::benchmark::State&) override
	{
		stats_hub.reset();
	}

public:
	std::shared_ptr<Kernel::StatisticsHub> stats_hub;
};

BENCHMARK_DEFINE_F(ErrorHandler_Fixture, ChecksumFailure)(benchmark::State& state)
{
	auto ec = make_error_code(ErrorCodes::Protocol_ErrorCodes::ChecksumFailure);
	for ([[maybe_unused]] auto _ : state)
	{
		Protocol::ProtocolHandler_ReadOp_ErrorHandler(ec, stats_hub);
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(ErrorHandler_Fixture, ChecksumFailure)->Unit(benchmark::kNanosecond);

BENCHMARK_DEFINE_F(ErrorHandler_Fixture, InvalidPacketFormat)(benchmark::State& state)
{
	auto ec = make_error_code(ErrorCodes::Protocol_ErrorCodes::InvalidPacketFormat);
	for ([[maybe_unused]] auto _ : state)
	{
		Protocol::ProtocolHandler_ReadOp_ErrorHandler(ec, stats_hub);
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(ErrorHandler_Fixture, InvalidPacketFormat)->Unit(benchmark::kNanosecond);

BENCHMARK_DEFINE_F(ErrorHandler_Fixture, OverlappingPackets)(benchmark::State& state)
{
	auto ec = make_error_code(ErrorCodes::Protocol_ErrorCodes::OverlappingPackets);
	for ([[maybe_unused]] auto _ : state)
	{
		Protocol::ProtocolHandler_ReadOp_ErrorHandler(ec, stats_hub);
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(ErrorHandler_Fixture, OverlappingPackets)->Unit(benchmark::kNanosecond);

BENCHMARK_DEFINE_F(ErrorHandler_Fixture, WaitingForMoreData)(benchmark::State& state)
{
	auto ec = make_error_code(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData);
	for ([[maybe_unused]] auto _ : state)
	{
		Protocol::ProtocolHandler_ReadOp_ErrorHandler(ec, stats_hub);
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(ErrorHandler_Fixture, WaitingForMoreData)->Unit(benchmark::kNanosecond);

BENCHMARK_DEFINE_F(ErrorHandler_Fixture, WithoutStatistics)(benchmark::State& state)
{
	auto ec = make_error_code(ErrorCodes::Protocol_ErrorCodes::ChecksumFailure);
	for ([[maybe_unused]] auto _ : state)
	{
		Protocol::ProtocolHandler_ReadOp_ErrorHandler(ec, nullptr);
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(ErrorHandler_Fixture, WithoutStatistics)->Unit(benchmark::kNanosecond);
