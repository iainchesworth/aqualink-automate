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

	/// Serialize an optional Temperature with freshness metadata. Returns null JSON when not
	/// measured; otherwise emits celsius/fahrenheit plus `last_updated` (epoch seconds, the
	/// codebase's time convention - null if never stamped) and a `stale` flag. The controller only
	/// reports temperatures while the pump runs (and only for the active body on a combo system),
	/// so a value can outlive its freshness; `stale` lets consumers reflect that without dropping
	/// the last-known reading.
	inline nlohmann::json SerializeTemperature(
		const std::optional<Kernel::Temperature>& temp,
		const std::optional<std::chrono::system_clock::time_point>& last_updated,
		bool stale)
	{
		if (!temp.has_value())
		{
			return nullptr;
		}

		nlohmann::json j = SerializeTemperature(*temp);

		if (last_updated.has_value())
		{
			j["last_updated"] = std::chrono::duration_cast<std::chrono::seconds>(last_updated->time_since_epoch()).count();
		}
		else
		{
			j["last_updated"] = nullptr;
		}

		j["stale"] = stale;

		return j;
	}

}
// namespace AqualinkAutomate::Utility
