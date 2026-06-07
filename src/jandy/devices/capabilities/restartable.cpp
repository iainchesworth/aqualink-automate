#include <algorithm>

#include "devices/capabilities/restartable.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices::Capabilities
{
	const std::chrono::seconds Restartable::DEFAULT_WATCHDOG_TIMEOUT{ std::chrono::seconds(30) };

	std::vector<Restartable*> Restartable::s_Instances;

	Restartable::Restartable(std::chrono::seconds timeout_in_seconds, bool delayed_start) :
		m_TimeoutDuration(timeout_in_seconds),
		m_IsRunning(false),
		m_LastKick()
	{
		s_Instances.push_back(this);

		if (!delayed_start)
		{
			Start();
		}
	}

	Restartable::~Restartable()
	{
		Stop();
		std::erase(s_Instances, this);
	}

	void Restartable::PollAll()
	{
		// A copy is not required: CheckWatchdog() never adds or removes instances.
		for (auto* instance : s_Instances)
		{
			instance->CheckWatchdog();
		}
	}

	void Restartable::Start()
	{
		if (m_IsRunning)
		{
			LogTrace(Channel::Devices, "Attempted to start watchdog that is already running; ignoring start request");
		}
		else
		{
			m_IsRunning = true;

			// Note the order here....if m_Running is _NOT_ true, Kick() will not action the request...

			Kick();
		}
	}

	void Restartable::Kick()
	{
		if (m_IsRunning)
		{
			m_LastKick = Now();
		}
		else
		{
			LogTrace(Channel::Devices, "Attempted to kick watchdog that is stopped; ignoring kick request");
		}
	}

	void Restartable::Stop()
	{
		m_IsRunning = false;
	}

	void Restartable::CheckWatchdog()
	{
		if (m_IsRunning && ((Now() - m_LastKick) > m_TimeoutDuration))
		{
			// No message has arrived within the timeout duration; mark this device as not operating.
			LogWarning(Channel::Devices, "Device timeout: device watchdog has expired");
			m_IsRunning = false;
			WatchdogTimeoutOccurred();
		}
	}

	std::chrono::seconds Restartable::GetTimeout() const
	{
		return m_TimeoutDuration;
	}

	bool Restartable::IsRunning() const
	{
		return m_IsRunning;
	}

	std::chrono::steady_clock::time_point Restartable::Now() const
	{
		return std::chrono::steady_clock::now();
	}

}
// namespace AqualinkAutomate::Devices::Capabilities
