#pragma once

#include <chrono>
#include <optional>

#include <nlohmann/json.hpp>

#include "kernel/temperature.h"
#include "utility/latency_percentile_tracker.h"

namespace AqualinkAutomate::Utility
{

	/// Convert nanoseconds to microseconds for JSON output.
	inline double NanosToMicros(std::chrono::nanoseconds ns) noexcept
	{
		return static_cast<double>(ns.count()) / 1000.0;
	}

	/// Serialize a latency percentile snapshot to JSON.
	inline nlohmann::json SerializeLatencySnapshot(const LatencyPercentileTracker<>::PercentileSnapshot& snapshot)
	{
		return {
			{"p1_us", NanosToMicros(snapshot.p1)},
			{"p50_us", NanosToMicros(snapshot.p50)},
			{"p95_us", NanosToMicros(snapshot.p95)},
			{"p99_us", NanosToMicros(snapshot.p99)},
			{"min_us", NanosToMicros(snapshot.min)},
			{"max_us", NanosToMicros(snapshot.max)},
			{"alltime_min_us", NanosToMicros(snapshot.alltime_min)},
			{"alltime_max_us", NanosToMicros(snapshot.alltime_max)},
			{"mean_us", NanosToMicros(snapshot.mean)},
			{"sample_count", snapshot.sample_count}
		};
	}

	/// Serialize a Temperature to JSON with celsius and fahrenheit fields.
	inline nlohmann::json SerializeTemperature(const Kernel::Temperature& temp)
	{
		return {
			{"celsius", temp.InCelsius().value()},
			{"fahrenheit", temp.InFahrenheit().value()}
		};
	}

	/// Serialize an optional Temperature. Returns null JSON when not measured.
	inline nlohmann::json SerializeTemperature(const std::optional<Kernel::Temperature>& temp)
	{
		if (!temp.has_value())
		{
			return nullptr;
		}

		return SerializeTemperature(*temp);
	}

}
// namespace AqualinkAutomate::Utility
