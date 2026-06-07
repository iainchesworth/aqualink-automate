#pragma once

#include <chrono>
#include <vector>

namespace AqualinkAutomate::Devices::Capabilities
{

	class Restartable
	{
		static const std::chrono::seconds DEFAULT_WATCHDOG_TIMEOUT;

	protected:
		Restartable(std::chrono::seconds timeout_in_seconds = DEFAULT_WATCHDOG_TIMEOUT, bool delayed_start = false);
		virtual ~Restartable();

	public:
		// Evaluates the watchdog deadline for every live Restartable instance and fires
		// WatchdogTimeoutOccurred() on any that have expired. Call once per frame.
		static void PollAll();

	protected:
		void Start();
		void Kick();
		void Stop();

	protected:
		std::chrono::seconds GetTimeout() const;
		bool IsRunning() const;

	protected:
		// Fired when the watchdog deadline elapses without an intervening Kick().
		virtual void WatchdogTimeoutOccurred() = 0;

		// Clock used for deadline evaluation; overridable so tests can inject time.
		virtual std::chrono::steady_clock::time_point Now() const;

		// Evaluates this instance's deadline; fires WatchdogTimeoutOccurred() on expiry.
		void CheckWatchdog();

	private:
		const std::chrono::seconds m_TimeoutDuration;
		bool m_IsRunning;
		std::chrono::steady_clock::time_point m_LastKick;

		static std::vector<Restartable*> s_Instances;
	};

}
// namespace AqualinkAutomate::Devices::Capabilities
