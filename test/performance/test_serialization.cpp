#include <iostream>
#include <cstddef>
#include <random>
#include <span>
#include <vector>

#include <benchmark/benchmark.h>

#include "jandy/messages/jandy_message.h"
#include "jandy/messages/jandy_message_constants.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_message.h"

using namespace AqualinkAutomate::Messages;

class JandyMessage_Deserialise_Fixture : public benchmark::Fixture 
{
public:
	void SetUp(const ::benchmark::State& state) override
	{
		data.clear();

		data.emplace_back(std::byte(0x10)); // HEADER
		data.emplace_back(std::byte(0x02)); // HEADER
		data.emplace_back(std::byte(0x42)); // DESTINATION
		data.emplace_back(std::byte(0x03)); // COMMAND
		data.emplace_back(std::byte(0x01)); // LINE ID

		{
			std::random_device rd;
			std::mt19937 generator(rd());
			std::uniform_int_distribution<> distribution(32, 126);

			for (auto i = 0; i < state.range(0); i++)
			{
				data.emplace_back(std::byte(distribution(generator)));
			}
		}

		data.emplace_back(std::byte(0x10)); // FOOTER
		data.emplace_back(std::byte(0x03)); // FOOTER

		span_data = std::span<const std::byte>(data.data(), data.size());
	}

	void TearDown(const ::benchmark::State& state) override
	{
	}

public:
	std::vector<std::byte> data;
	std::span<const std::byte> span_data;
};

BENCHMARK_DEFINE_F(JandyMessage_Deserialise_Fixture, JandyMessage_Deserialise)(benchmark::State& state)
{
	// Construct the message here (at benchmark run time), NOT as a fixture member.
	// BENCHMARK_REGISTER_F instantiates the fixture object during static
	// initialisation, and JandyMessage_Message's constructor connects to a global
	// boost::signals2 signal via IMessageSignalRecv::GetSignal(). Under MSVC
	// Release + LTO that function-local-static signal is not yet initialised at
	// static-init time, so constructing the message as a member dereferenced a
	// null signal and crashed the perf executable before main (0xC0000005).
	// Building it inside the benchmark body defers construction to run time.
	JandyMessage_Message message;

	for ([[maybe_unused]] auto _ : state)
	{
		benchmark::DoNotOptimize(message.Deserialize(span_data));
		benchmark::ClobberMemory();
	}

	state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(state.range(0) + JandyMessage::MINIMUM_PACKET_LENGTH));
}

BENCHMARK_REGISTER_F(JandyMessage_Deserialise_Fixture, JandyMessage_Deserialise)->DenseRange(1, 120, 12)->Unit(benchmark::kMillisecond);
