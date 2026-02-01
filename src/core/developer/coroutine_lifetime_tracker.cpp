#include <format>

#include "developer/coroutine_lifetime_tracker.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Developer
{

	CoroutineLifetimeTracker& CoroutineLifetimeTracker::Instance()
	{
		static CoroutineLifetimeTracker instance;
		return instance;
	}

	void CoroutineLifetimeTracker::OnCreated(uintptr_t handle, const std::string& name, const std::source_location& location)
	{
		auto now = std::chrono::steady_clock::now();
		auto loc_str = std::format("{}:{}", location.file_name(), location.line());

		{
			std::unique_lock lock(m_Mutex);
			m_ActiveCoroutines.emplace(handle, CoroutineInfo{
				.handle_address = handle,
				.creation_location = std::move(loc_str),
				.created_at = now,
				.last_state_change = now,
				.suspend_count = 0,
				.resume_count = 0,
				.is_suspended = false,
				.name = name
			});
		}

		m_TotalCreated.fetch_add(1, std::memory_order_relaxed);

		LogTrace(Channel::Developer, std::format("Coroutine created: '{}' at 0x{:X}", name, handle));
	}

	void CoroutineLifetimeTracker::OnSuspended(uintptr_t handle)
	{
		std::unique_lock lock(m_Mutex);
		if (auto it = m_ActiveCoroutines.find(handle); it != m_ActiveCoroutines.end())
		{
			it->second.is_suspended = true;
			it->second.suspend_count++;
			it->second.last_state_change = std::chrono::steady_clock::now();
		}
	}

	void CoroutineLifetimeTracker::OnResumed(uintptr_t handle)
	{
		std::unique_lock lock(m_Mutex);
		if (auto it = m_ActiveCoroutines.find(handle); it != m_ActiveCoroutines.end())
		{
			it->second.is_suspended = false;
			it->second.resume_count++;
			it->second.last_state_change = std::chrono::steady_clock::now();
		}
	}

	void CoroutineLifetimeTracker::OnDestroyed(uintptr_t handle)
	{
		{
			std::unique_lock lock(m_Mutex);
			m_ActiveCoroutines.erase(handle);
		}

		m_TotalDestroyed.fetch_add(1, std::memory_order_relaxed);

		LogTrace(Channel::Developer, std::format("Coroutine destroyed: 0x{:X}", handle));
	}

	uint64_t CoroutineLifetimeTracker::ActiveCount() const
	{
		std::shared_lock lock(m_Mutex);
		return m_ActiveCoroutines.size();
	}

	uint64_t CoroutineLifetimeTracker::TotalCreated() const
	{
		return m_TotalCreated.load(std::memory_order_relaxed);
	}

	uint64_t CoroutineLifetimeTracker::TotalDestroyed() const
	{
		return m_TotalDestroyed.load(std::memory_order_relaxed);
	}

	std::vector<CoroutineInfo> CoroutineLifetimeTracker::GetActive() const
	{
		std::shared_lock lock(m_Mutex);
		std::vector<CoroutineInfo> result;
		result.reserve(m_ActiveCoroutines.size());
		for (const auto& [_, info] : m_ActiveCoroutines)
		{
			result.push_back(info);
		}
		return result;
	}

	std::vector<CoroutineInfo> CoroutineLifetimeTracker::GetSuspendedLongerThan(std::chrono::steady_clock::duration threshold) const
	{
		auto now = std::chrono::steady_clock::now();
		std::shared_lock lock(m_Mutex);
		std::vector<CoroutineInfo> result;
		for (const auto& [_, info] : m_ActiveCoroutines)
		{
			if (info.is_suspended && (now - info.last_state_change) > threshold)
			{
				result.push_back(info);
			}
		}
		return result;
	}

	void CoroutineLifetimeTracker::DumpActive() const
	{
		auto active = GetActive();
		LogInfo(Channel::Developer, std::format("=== Active Coroutines ({}) ===", active.size()));
		auto now = std::chrono::steady_clock::now();
		for (const auto& info : active)
		{
			auto age_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - info.created_at).count();
			LogInfo(Channel::Developer, std::format(
				"  [0x{:X}] '{}' | {} | age={}ms | suspends={} resumes={} | from {}",
				info.handle_address,
				info.name,
				info.is_suspended ? "SUSPENDED" : "RUNNING",
				age_ms,
				info.suspend_count,
				info.resume_count,
				info.creation_location
			));
		}
	}

	void CoroutineLifetimeTracker::DumpSuspendedLongerThan(std::chrono::steady_clock::duration threshold) const
	{
		auto stuck = GetSuspendedLongerThan(threshold);
		auto threshold_ms = std::chrono::duration_cast<std::chrono::milliseconds>(threshold).count();
		LogInfo(Channel::Developer, std::format("=== Coroutines Suspended > {}ms ({}) ===", threshold_ms, stuck.size()));
		auto now = std::chrono::steady_clock::now();
		for (const auto& info : stuck)
		{
			auto suspended_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - info.last_state_change).count();
			LogInfo(Channel::Developer, std::format(
				"  [0x{:X}] '{}' | suspended for {}ms | from {}",
				info.handle_address,
				info.name,
				suspended_ms,
				info.creation_location
			));
		}
	}

}
// namespace AqualinkAutomate::Developer
