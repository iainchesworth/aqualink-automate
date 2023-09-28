#pragma once

#include <atomic>
#include <chrono>

#include <boost/asio/steady_timer.hpp>

#include "types/asynchronous_executor.h"

namespace AqualinkAutomate::Devices::Capabilities
{

	class Restartable
	{
		static const std::chrono::seconds DEFAULT_WATCHDOG_TIMEOUT;

	protected:
		Restartable(Types::ExecutorType executor, std::chrono::seconds timeout_in_seconds = DEFAULT_WATCHDOG_TIMEOUT, bool delayed_start = false);
		virtual ~Restartable();

	protected:
		void Start();
		void Kick();
		void Stop();

	protected:
		std::chrono::seconds GetTimeout() const;
		bool IsRunning() const;

	protected:
		virtual void WatchdogTimeoutOccurred() = 0;

	private:
		const std::chrono::seconds m_TimeoutDuration;
		boost::asio::steady_timer m_WatchdogTimer;

	private:
		std::atomic<bool> m_IsRunning;
	};

}
// namespace AqualinkAutomate::Devices::Capabilities
