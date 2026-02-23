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

	struct SignalConnectionInfo
	{
		uint64_t id{0};
		std::string creation_location;
		std::string description;
		std::chrono::steady_clock::time_point created_at;
		bool is_disconnected{false};
		std::chrono::steady_clock::time_point disconnected_at;
	};

	class SignalConnectionTracker
	{
	public:
		static SignalConnectionTracker& Instance();

		SignalConnectionTracker(const SignalConnectionTracker&) = delete;
		SignalConnectionTracker& operator=(const SignalConnectionTracker&) = delete;

		uint64_t OnConnected(const std::string& description, const std::source_location& location = std::source_location::current());
		void OnDisconnected(uint64_t id);

		uint64_t ActiveCount() const;

		std::vector<SignalConnectionInfo> GetActive() const;
		std::vector<SignalConnectionInfo> GetOlderThan(std::chrono::steady_clock::duration threshold) const;

		void DumpActive() const;
		void DumpOlderThan(std::chrono::steady_clock::duration threshold) const;

	private:
		SignalConnectionTracker() = default;

		mutable std::shared_mutex m_Mutex;
		std::unordered_map<uint64_t, SignalConnectionInfo> m_Connections;
		std::atomic<uint64_t> m_NextId{1};
	};

}
// namespace AqualinkAutomate::Developer
