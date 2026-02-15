#include <format>

#include <magic_enum/magic_enum.hpp>

#include "developer/async_operation_tracker.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Developer
{

	AsyncOperationTracker& AsyncOperationTracker::Instance()
	{
		static AsyncOperationTracker instance;
		return instance;
	}

	uint64_t AsyncOperationTracker::OnStarted(AsyncOperationType type, const std::string& description, const std::source_location& location)
	{
		auto now = std::chrono::steady_clock::now();
		auto id = m_NextId.fetch_add(1, std::memory_order_relaxed);
		auto loc_str = std::format("{}:{}", location.file_name(), location.line());

		{
			std::unique_lock lock(m_Mutex);
			m_Operations.emplace(id, AsyncOperationInfo{
				.id = id,
				.type = type,
				.description = description,
				.creation_location = std::move(loc_str),
				.started_at = now,
				.is_completed = false,
				.was_cancelled = false,
				.had_error = false,
				.completed_at = {}
			});
		}

		LogTrace(Channel::Developer, std::format("Async op started: id={} type={} '{}'", id, magic_enum::enum_name(type), description));

		return id;
	}

	void AsyncOperationTracker::OnCompleted(uint64_t id, bool cancelled, bool error)
	{
		if (0 == id)
		{
			return;
		}

		std::unique_lock lock(m_Mutex);
		if (auto it = m_Operations.find(id); it != m_Operations.end() && !it->second.is_completed)
		{
			it->second.is_completed = true;
			it->second.was_cancelled = cancelled;
			it->second.had_error = error;
			it->second.completed_at = std::chrono::steady_clock::now();

			LogTrace(Channel::Developer, std::format("Async op completed: id={} cancelled={} error={}", id, cancelled, error));
		}
	}

	uint64_t AsyncOperationTracker::PendingCount() const
	{
		std::shared_lock lock(m_Mutex);
		uint64_t count = 0;
		for (const auto& [_, info] : m_Operations)
		{
			if (!info.is_completed)
			{
				++count;
			}
		}
		return count;
	}

	uint64_t AsyncOperationTracker::PendingCountByType(AsyncOperationType type) const
	{
		std::shared_lock lock(m_Mutex);
		uint64_t count = 0;
		for (const auto& [_, info] : m_Operations)
		{
			if (!info.is_completed && info.type == type)
			{
				++count;
			}
		}
		return count;
	}

	std::vector<AsyncOperationInfo> AsyncOperationTracker::GetPending() const
	{
		std::shared_lock lock(m_Mutex);
		std::vector<AsyncOperationInfo> result;
		for (const auto& [_, info] : m_Operations)
		{
			if (!info.is_completed)
			{
				result.push_back(info);
			}
		}
		return result;
	}

	std::vector<AsyncOperationInfo> AsyncOperationTracker::GetStuck(std::chrono::steady_clock::duration threshold) const
	{
		auto now = std::chrono::steady_clock::now();
		std::shared_lock lock(m_Mutex);
		std::vector<AsyncOperationInfo> result;
		for (const auto& [_, info] : m_Operations)
		{
			if (!info.is_completed && (now - info.started_at) > threshold)
			{
				result.push_back(info);
			}
		}
		return result;
	}

	void AsyncOperationTracker::DumpPending() const
	{
		auto pending = GetPending();
		LogInfo(Channel::Developer, std::format("=== Pending Async Operations ({}) ===", pending.size()));
		auto now = std::chrono::steady_clock::now();
		for (const auto& info : pending)
		{
			auto age_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - info.started_at).count();
			LogInfo(Channel::Developer, std::format(
				"  [id={}] type={} '{}' | pending for {}ms | from {}",
				info.id,
				magic_enum::enum_name(info.type),
				info.description,
				age_ms,
				info.creation_location
			));
		}
	}

	void AsyncOperationTracker::DumpStuck(std::chrono::steady_clock::duration threshold) const
	{
		auto stuck = GetStuck(threshold);
		auto threshold_ms = std::chrono::duration_cast<std::chrono::milliseconds>(threshold).count();
		LogInfo(Channel::Developer, std::format("=== Stuck Async Operations > {}ms ({}) ===", threshold_ms, stuck.size()));
		auto now = std::chrono::steady_clock::now();
		for (const auto& info : stuck)
		{
			auto age_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - info.started_at).count();
			LogInfo(Channel::Developer, std::format(
				"  [id={}] type={} '{}' | stuck for {}ms | from {}",
				info.id,
				magic_enum::enum_name(info.type),
				info.description,
				age_ms,
				info.creation_location
			));
		}
	}

	// --- ScopedOp ---

	ScopedOp::ScopedOp(AsyncOperationType type, const std::string& description, const std::source_location& location)
		: m_Id(AsyncOperationTracker::Instance().OnStarted(type, description, location))
		, m_Completed(false)
	{
	}

	ScopedOp::~ScopedOp()
	{
		if (!m_Completed)
		{
			AsyncOperationTracker::Instance().OnCompleted(m_Id);
		}
	}

	ScopedOp::ScopedOp(ScopedOp&& other) noexcept
		: m_Id(other.m_Id)
		, m_Completed(other.m_Completed)
	{
		other.m_Completed = true; // Prevent the moved-from object from completing
	}

	void ScopedOp::Complete(bool cancelled, bool error)
	{
		if (!m_Completed)
		{
			AsyncOperationTracker::Instance().OnCompleted(m_Id, cancelled, error);
			m_Completed = true;
		}
	}

}
// namespace AqualinkAutomate::Developer
