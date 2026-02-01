#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <shared_mutex>
#include <source_location>
#include <string>
#include <unordered_map>
#include <vector>

namespace AqualinkAutomate::Developer
{

	struct CoroutineInfo
	{
		uintptr_t handle_address{0};
		std::string creation_location;
		std::chrono::steady_clock::time_point created_at;
		std::chrono::steady_clock::time_point last_state_change;
		uint64_t suspend_count{0};
		uint64_t resume_count{0};
		bool is_suspended{false};
		std::string name;
	};

	class CoroutineLifetimeTracker
	{
	public:
		static CoroutineLifetimeTracker& Instance();

		CoroutineLifetimeTracker(const CoroutineLifetimeTracker&) = delete;
		CoroutineLifetimeTracker& operator=(const CoroutineLifetimeTracker&) = delete;

		void OnCreated(uintptr_t handle, const std::string& name, const std::source_location& location = std::source_location::current());
		void OnSuspended(uintptr_t handle);
		void OnResumed(uintptr_t handle);
		void OnDestroyed(uintptr_t handle);

		uint64_t ActiveCount() const;
		uint64_t TotalCreated() const;
		uint64_t TotalDestroyed() const;

		std::vector<CoroutineInfo> GetActive() const;
		std::vector<CoroutineInfo> GetSuspendedLongerThan(std::chrono::steady_clock::duration threshold) const;

		void DumpActive() const;
		void DumpSuspendedLongerThan(std::chrono::steady_clock::duration threshold) const;

	private:
		CoroutineLifetimeTracker() = default;

		mutable std::shared_mutex m_Mutex;
		std::unordered_map<uintptr_t, CoroutineInfo> m_ActiveCoroutines;
		std::atomic<uint64_t> m_TotalCreated{0};
		std::atomic<uint64_t> m_TotalDestroyed{0};
	};

}
// namespace AqualinkAutomate::Developer
