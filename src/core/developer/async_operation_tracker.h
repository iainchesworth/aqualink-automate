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

	enum class AsyncOperationType
	{
		SerialRead,
		SerialWrite,
		TimerWait,
		SignalAwait,
		WebSocketRead,
		WebSocketWrite,
		HttpAccept,
		MqttPublish,
		Other
	};

	struct AsyncOperationInfo
	{
		uint64_t id{0};
		AsyncOperationType type{AsyncOperationType::Other};
		std::string description;
		std::string creation_location;
		std::chrono::steady_clock::time_point started_at;
		bool is_completed{false};
		bool was_cancelled{false};
		bool had_error{false};
		std::chrono::steady_clock::time_point completed_at;
	};

	class AsyncOperationTracker
	{
	public:
		static AsyncOperationTracker& Instance();

		AsyncOperationTracker(const AsyncOperationTracker&) = delete;
		AsyncOperationTracker& operator=(const AsyncOperationTracker&) = delete;

		uint64_t OnStarted(AsyncOperationType type, const std::string& description, const std::source_location& location = std::source_location::current());
		void OnCompleted(uint64_t id, bool cancelled = false, bool error = false);

		uint64_t PendingCount() const;
		uint64_t PendingCountByType(AsyncOperationType type) const;

		std::vector<AsyncOperationInfo> GetPending() const;
		std::vector<AsyncOperationInfo> GetStuck(std::chrono::steady_clock::duration threshold) const;

		void DumpPending() const;
		void DumpStuck(std::chrono::steady_clock::duration threshold) const;

	private:
		AsyncOperationTracker() = default;

		mutable std::shared_mutex m_Mutex;
		std::unordered_map<uint64_t, AsyncOperationInfo> m_Operations;
		std::atomic<uint64_t> m_NextId{1};
	};

	class ScopedOp
	{
	public:
		ScopedOp(AsyncOperationType type, const std::string& description, const std::source_location& location = std::source_location::current());
		~ScopedOp();

		ScopedOp(const ScopedOp&) = delete;
		ScopedOp& operator=(const ScopedOp&) = delete;
		ScopedOp(ScopedOp&& other) noexcept;
		ScopedOp& operator=(ScopedOp&&) = delete;

		void Complete(bool cancelled = false, bool error = false);
		uint64_t Id() const { return m_Id; }

	private:
		uint64_t m_Id{0};
		bool m_Completed{false};
	};

}
// namespace AqualinkAutomate::Developer
