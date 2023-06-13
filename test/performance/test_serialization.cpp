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

public:
	JandyMessage_Message message;
};

BENCHMARK_DEFINE_F(JandyMessage_Deserialise_Fixture, JandyMessage_Deserialise)(benchmark::State& state)
{
	for (auto _ : state)
	{
		benchmark::DoNotOptimize(message.Deserialize(span_data));
		benchmark::ClobberMemory();
	}

	state.SetBytesProcessed(uint64_t(state.iterations()) * uint64_t(state.range(0) + JandyMessage::MINIMUM_PACKET_LENGTH));
}

BENCHMARK_REGISTER_F(JandyMessage_Deserialise_Fixture, JandyMessage_Deserialise)->DenseRange(1, 120, 12)->Unit(benchmark::kMillisecond);
