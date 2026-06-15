#pragma once

#include <chrono>
#include <cstddef>
#include <deque>
#include <string>

#include <nlohmann/json_fwd.hpp>

namespace AqualinkAutomate::Devices::Capabilities
{

	// Protocol-agnostic recent-command log. A controller that can be commanded
	// (actuate equipment, set a setpoint, drive the chlorinator, navigate pages, ...)
	// mixes this in so the UI can show "what it recently did". The CommandDispatcher
	// records each dispatched command -- and its outcome -- on the controller it routed
	// to, at the single command chokepoint, so every command-capable controller gets a
	// uniform history without per-device plumbing. Shared by all protocols, hence it
	// lives in the core library (mirrors Restartable).
	//
	// The mixin is a plain data holder (non-polymorphic); call sites reach it by a
	// dynamic_cast from a polymorphic capability interface on the same object, so it
	// needs no virtuals of its own.
	class CommandHistory
	{
	public:
		// Newest-last ring-buffer bound; the oldest entries are evicted past this.
		static constexpr std::size_t MAX_ENTRIES = 16U;

		struct Entry
		{
			std::chrono::system_clock::time_point timestamp;
			std::string description;   // e.g. "toggle 'Pool Light'"
			std::string outcome;       // CommandResult enumerator name, e.g. "Success"
		};

		// Append a command to the history (FIFO eviction once MAX_ENTRIES is exceeded).
		void RecordCommand(std::string description, std::string outcome);

		// JSON array (oldest first) of { ts, age_seconds, description, outcome }, where
		// ts is Unix epoch seconds and age_seconds is measured against the current time.
		nlohmann::json DescribeRecentCommands() const;

	protected:
		// Only ever destroyed as part of the owning device, never through this base.
		~CommandHistory() = default;

	private:
		std::deque<Entry> m_Entries;
	};

}
// namespace AqualinkAutomate::Devices::Capabilities
