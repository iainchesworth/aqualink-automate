#include <format>

#include "developer/signal_connection_tracker.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Developer
{

	SignalConnectionTracker& SignalConnectionTracker::Instance()
	{
		static SignalConnectionTracker instance;
		return instance;
	}

	uint64_t SignalConnectionTracker::OnConnected(const std::string& description, const std::source_location& location)
	{
		auto now = std::chrono::steady_clock::now();
		auto id = m_NextId.fetch_add(1, std::memory_order_relaxed);
		auto loc_str = std::format("{}:{}", location.file_name(), location.line());

		{
			std::unique_lock lock(m_Mutex);
			m_Connections.emplace(id, SignalConnectionInfo{
				.id = id,
				.creation_location = std::move(loc_str),
				.description = description,
				.created_at = now,
				.is_disconnected = false,
				.disconnected_at = {}
			});
		}

		LogTrace(Channel::Developer, std::format("Signal connection created: id={} '{}'", id, description));

		return id;
	}

	void SignalConnectionTracker::OnDisconnected(uint64_t id)
	{
		if (0 == id)
		{
			return;
		}

		std::unique_lock lock(m_Mutex);
		if (auto it = m_Connections.find(id); it != m_Connections.end() && !it->second.is_disconnected)
		{
			it->second.is_disconnected = true;
			it->second.disconnected_at = std::chrono::steady_clock::now();

			LogTrace(Channel::Developer, std::format("Signal connection disconnected: id={}", id));
		}
	}

	uint64_t SignalConnectionTracker::ActiveCount() const
	{
		std::shared_lock lock(m_Mutex);
		uint64_t count = 0;
		for (const auto& [_, info] : m_Connections)
		{
			if (!info.is_disconnected)
			{
				++count;
			}
		}
		return count;
	}

	std::vector<SignalConnectionInfo> SignalConnectionTracker::GetActive() const
	{
		std::shared_lock lock(m_Mutex);
		std::vector<SignalConnectionInfo> result;
		for (const auto& [_, info] : m_Connections)
		{
			if (!info.is_disconnected)
			{
				result.push_back(info);
			}
		}
		return result;
	}

	std::vector<SignalConnectionInfo> SignalConnectionTracker::GetOlderThan(std::chrono::steady_clock::duration threshold) const
	{
		auto now = std::chrono::steady_clock::now();
		std::shared_lock lock(m_Mutex);
		std::vector<SignalConnectionInfo> result;
		for (const auto& [_, info] : m_Connections)
		{
			if (!info.is_disconnected && (now - info.created_at) > threshold)
			{
				result.push_back(info);
			}
		}
		return result;
	}

	void SignalConnectionTracker::DumpActive() const
	{
		auto active = GetActive();
		LogInfo(Channel::Developer, std::format("=== Active Signal Connections ({}) ===", active.size()));
		auto now = std::chrono::steady_clock::now();
		for (const auto& info : active)
		{
			auto age_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - info.created_at).count();
			LogInfo(Channel::Developer, std::format(
				"  [id={}] '{}' | age={}ms | from {}",
				info.id,
				info.description,
				age_ms,
				info.creation_location
			));
		}
	}

	void SignalConnectionTracker::DumpOlderThan(std::chrono::steady_clock::duration threshold) const
	{
		auto old = GetOlderThan(threshold);
		auto threshold_ms = std::chrono::duration_cast<std::chrono::milliseconds>(threshold).count();
		LogInfo(Channel::Developer, std::format("=== Signal Connections Older Than {}ms ({}) ===", threshold_ms, old.size()));
		auto now = std::chrono::steady_clock::now();
		for (const auto& info : old)
		{
			auto age_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - info.created_at).count();
			LogInfo(Channel::Developer, std::format(
				"  [id={}] '{}' | age={}ms | from {}",
				info.id,
				info.description,
				age_ms,
				info.creation_location
			));
		}
	}

}
// namespace AqualinkAutomate::Developer
