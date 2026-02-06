#include <array>
#include <cstddef>
#include <iostream>
#include <memory>
#include <random>
#include <ranges>
#include <span>
#include <vector>

#include <benchmark/benchmark.h>

#include "jandy/factories/jandy_message_factory.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/jandy_message.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_message.h"
#include "jandy/messages/jandy_message_status.h"
#include "jandy/messages/jandy_message_probe.h"
#include "jandy/messages/jandy_message_ids.h"

using namespace AqualinkAutomate::Factory;
using namespace AqualinkAutomate::Messages;
using namespace AqualinkAutomate::Types;

static void BM_Factory_CreateMessage_Ack(benchmark::State& state)
{
    for ([[maybe_unused]] auto _ : state)
    {
        auto message = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::Ack);
        benchmark::DoNotOptimize(message);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Factory_CreateMessage_Ack);

static void BM_Factory_CreateMessage_Status(benchmark::State& state)
{
    for ([[maybe_unused]] auto _ : state)
    {
        auto message = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::Status);
        benchmark::DoNotOptimize(message);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Factory_CreateMessage_Status);

static void BM_Factory_CreateMessage_Probe(benchmark::State& state)
{
    for ([[maybe_unused]] auto _ : state)
    {
        auto message = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::Probe);
        benchmark::DoNotOptimize(message);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Factory_CreateMessage_Probe);

static void BM_Factory_CreateMessage_Message(benchmark::State& state)
{
    for ([[maybe_unused]] auto _ : state)
    {
        auto message = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::Message);
        benchmark::DoNotOptimize(message);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Factory_CreateMessage_Message);

static void BM_Factory_CreateMessage_Unknown(benchmark::State& state)
{
    for ([[maybe_unused]] auto _ : state)
    {
        auto message = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::Unknown);
        benchmark::DoNotOptimize(message);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Factory_CreateMessage_Unknown);

static void BM_Factory_CreateMessage_IAQPoll(benchmark::State& state)
{
    for ([[maybe_unused]] auto _ : state)
    {
        auto message = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::IAQ_Poll);
        benchmark::DoNotOptimize(message);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Factory_CreateMessage_IAQPoll);

class Factory_HotVsCold_Fixture : public benchmark::Fixture
{
public:
    void SetUp(const ::benchmark::State& state) override
    {
        // Hot path IDs
        hot_ids = {
            JandyMessageIds::Ack,
            JandyMessageIds::Status,
            JandyMessageIds::Probe
        };

        // Cold path IDs
        cold_ids = {
            JandyMessageIds::Message,
            JandyMessageIds::MessageLong,
            JandyMessageIds::Unknown,
            JandyMessageIds::AQUARITE_GetId,
            JandyMessageIds::IAQ_Poll
        };
    }

    std::vector<JandyMessageIds> hot_ids;
    std::vector<JandyMessageIds> cold_ids;
};

BENCHMARK_DEFINE_F(Factory_HotVsCold_Fixture, HotPath_Sequential)(benchmark::State& state)
{
    size_t idx = 0;
    for ([[maybe_unused]] auto _ : state)
    {
        auto message = JandyMessageFactoryT::CreateMessageFromRaw(hot_ids[idx % hot_ids.size()]);
        benchmark::DoNotOptimize(message);
        benchmark::ClobberMemory();
        ++idx;
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(Factory_HotVsCold_Fixture, HotPath_Sequential);

BENCHMARK_DEFINE_F(Factory_HotVsCold_Fixture, ColdPath_Sequential)(benchmark::State& state)
{
    size_t idx = 0;
    for ([[maybe_unused]] auto _ : state)
    {
        auto message = JandyMessageFactoryT::CreateMessageFromRaw(cold_ids[idx % cold_ids.size()]);
        benchmark::DoNotOptimize(message);
        benchmark::ClobberMemory();
        ++idx;
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(Factory_HotVsCold_Fixture, ColdPath_Sequential);

class Factory_RoundTrip_Fixed_Fixture : public benchmark::Fixture
{
public:
    void SetUp(const ::benchmark::State&) override
    {
        SetupAckMessage();
        SetupStatusMessage();
    }

    void SetupAckMessage()
    {
        JandyMessage_Ack ack(AckTypes::ACK_IAQTouch, 0x10);
        ack.Serialize(ack_data);
    }

    void SetupStatusMessage()
    {
        JandyMessage_Status status;
        status.Serialize(status_data);
    }

    std::vector<uint8_t> ack_data;
    std::vector<uint8_t> status_data;
};

BENCHMARK_DEFINE_F(Factory_RoundTrip_Fixed_Fixture, RoundTrip_Ack)(benchmark::State& state)
{
    for ([[maybe_unused]] auto _ : state)
    {
        auto result = JandyMessageFactoryT::CreateFromSerialData(std::ranges::subrange(ack_data.begin(), ack_data.end()));
        benchmark::DoNotOptimize(result);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * ack_data.size()));
}
BENCHMARK_REGISTER_F(Factory_RoundTrip_Fixed_Fixture, RoundTrip_Ack);

BENCHMARK_DEFINE_F(Factory_RoundTrip_Fixed_Fixture, RoundTrip_Status)(benchmark::State& state)
{
    for ([[maybe_unused]] auto _ : state)
    {
        auto result = JandyMessageFactoryT::CreateFromSerialData(std::ranges::subrange(status_data.begin(), status_data.end()));
        benchmark::DoNotOptimize(result);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * status_data.size()));
}
BENCHMARK_REGISTER_F(Factory_RoundTrip_Fixed_Fixture, RoundTrip_Status);

class Factory_RoundTrip_Message_Fixture : public benchmark::Fixture
{
public:
    void SetUp(const ::benchmark::State& state) override
    {
        const int64_t content_size = state.range(0); // always present for this fixture
        SetupMessageMessage(content_size > 0 ? content_size : 40);
    }

    void SetupMessageMessage(int64_t content_size)
    {
        std::string content;
        content.reserve(static_cast<size_t>(content_size));

        std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<> dist(32, 126);

        for (int64_t i = 0; i < content_size; ++i)
        {
            content.push_back(static_cast<char>(dist(gen)));
        }

        JandyMessage_Message msg(content);
        msg.Serialize(message_data);
    }

    std::vector<uint8_t> message_data;
};

BENCHMARK_DEFINE_F(Factory_RoundTrip_Message_Fixture, RoundTrip_Message)(benchmark::State& state)
{
    for ([[maybe_unused]] auto _ : state)
    {
        auto result = JandyMessageFactoryT::CreateFromSerialData(std::ranges::subrange(message_data.begin(), message_data.end()));
        benchmark::DoNotOptimize(result);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * message_data.size()));
}
BENCHMARK_REGISTER_F(Factory_RoundTrip_Message_Fixture, RoundTrip_Message)
->DenseRange(1, 120, 20)
->Unit(benchmark::kMicrosecond);

static void BM_Factory_BatchCreate_HotPath(benchmark::State& state)
{
    const auto batch_size = static_cast<int>(state.range(0));
    const std::array<JandyMessageIds, 3> hot_ids = {
        JandyMessageIds::Ack,
        JandyMessageIds::Status,
        JandyMessageIds::Probe
    };

    for ([[maybe_unused]] auto _ : state)
    {
        for (int i = 0; i < batch_size; ++i)
        {
            auto message = JandyMessageFactoryT::CreateMessageFromRaw(hot_ids[i % hot_ids.size()]);
            benchmark::DoNotOptimize(message);
        }
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_Factory_BatchCreate_HotPath)
->Arg(10)
->Arg(100)
->Arg(1000)
->Unit(benchmark::kMicrosecond);

static void BM_Factory_BatchCreate_Mixed(benchmark::State& state)
{
    const auto batch_size = static_cast<int>(state.range(0));
    const std::array<JandyMessageIds, 6> mixed_ids = {
        JandyMessageIds::Ack,         // Hot
        JandyMessageIds::Status,      // Hot
        JandyMessageIds::Message,     // Cold
        JandyMessageIds::Probe,       // Hot
        JandyMessageIds::Unknown,     // Cold
        JandyMessageIds::IAQ_Poll     // Cold
    };

    for ([[maybe_unused]] auto _ : state)
    {
        for (int i = 0; i < batch_size; ++i)
        {
            auto message = JandyMessageFactoryT::CreateMessageFromRaw(mixed_ids[i % mixed_ids.size()]);
            benchmark::DoNotOptimize(message);
        }
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_Factory_BatchCreate_Mixed)
->Arg(10)
->Arg(100)
->Arg(1000)
->Unit(benchmark::kMicrosecond);

class Factory_RealisticWorkload_Fixture : public benchmark::Fixture
{
public:
    void SetUp(const ::benchmark::State& state) override
    {
        // Simulate realistic message distribution:
        // 70% hot path (Ack, Status, Probe)
        // 30% cold path (Message, others)

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, 99);

        int64_t stream_size = (state.range(0) > 0) ? state.range(0) : 100;

        message_stream.clear();
        message_stream.reserve(stream_size);

        for (int64_t i = 0; i < stream_size; ++i)
        {
            int roll = dist(gen);
            if (roll < 40) {
                message_stream.push_back(JandyMessageIds::Ack);
            }
            else if (roll < 65) {
                message_stream.push_back(JandyMessageIds::Status);
            }
            else if (roll < 70) {
                message_stream.push_back(JandyMessageIds::Probe);
            }
            else if (roll < 85) {
                message_stream.push_back(JandyMessageIds::Message);
            }
            else if (roll < 95) {
                message_stream.push_back(JandyMessageIds::IAQ_Poll);
            }
            else {
                message_stream.push_back(JandyMessageIds::Unknown);
            }
        }
    }

    std::vector<JandyMessageIds> message_stream;
};

BENCHMARK_DEFINE_F(Factory_RealisticWorkload_Fixture, RealisticWorkload)(benchmark::State& state)
{
    for ([[maybe_unused]] auto _ : state)
    {
        for (const auto& id : message_stream)
        {
            auto message = JandyMessageFactoryT::CreateMessageFromRaw(id);
            benchmark::DoNotOptimize(message);
        }
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * message_stream.size()));
}
BENCHMARK_REGISTER_F(Factory_RealisticWorkload_Fixture, RealisticWorkload)
->Arg(100)
->Arg(1000)
->Arg(10000)
->Unit(benchmark::kMicrosecond);

static void BM_Factory_CacheEfficiency_HotOnly(benchmark::State& state)
{
    // Repeatedly create the same hot path message to test cache efficiency
    for ([[maybe_unused]] auto _ : state)
    {
        auto message = JandyMessageFactoryT::CreateMessageFromRaw(JandyMessageIds::Ack);
        benchmark::DoNotOptimize(message);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Factory_CacheEfficiency_HotOnly)->Threads(1)->Threads(4)->Threads(8);

static void BM_Factory_CacheEfficiency_Alternating(benchmark::State& state)
{
    // Alternate between hot and cold to test cache thrashing
    bool use_hot = true;
    for ([[maybe_unused]] auto _ : state)
    {
        auto id = use_hot ? JandyMessageIds::Ack : JandyMessageIds::Message;
        auto message = JandyMessageFactoryT::CreateMessageFromRaw(id);
        benchmark::DoNotOptimize(message);
        use_hot = !use_hot;
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Factory_CacheEfficiency_Alternating)->Threads(1)->Threads(4)->Threads(8);

static void BM_Factory_ErrorPath_InvalidMessageId(benchmark::State& state)
{
    for ([[maybe_unused]] auto _ : state)
    {
        auto message = JandyMessageFactoryT::CreateMessageFromRaw(static_cast<JandyMessageIds>(0xFF));
        benchmark::DoNotOptimize(message);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Factory_ErrorPath_InvalidMessageId);

static void BM_Factory_ErrorPath_TooShort(benchmark::State& state)
{
    std::vector<uint8_t> short_data = { 0x10 };

    for ([[maybe_unused]] auto _ : state)
    {
        auto result = JandyMessageFactoryT::CreateFromSerialData(std::ranges::subrange(short_data.begin(), short_data.end()));
        benchmark::DoNotOptimize(result);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Factory_ErrorPath_TooShort);
