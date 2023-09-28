#include <boost/system/error_code.hpp>

#include "jandy/devices/capabilities/restartable.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices::Capabilities
{
	const std::chrono::seconds Restartable::DEFAULT_WATCHDOG_TIMEOUT{ std::chrono::seconds(30) };

	Restartable::Restartable(Types::ExecutorType executor, std::chrono::seconds timeout_in_seconds, bool delayed_start) :
		m_TimeoutDuration(timeout_in_seconds),
		m_WatchdogTimer(executor),
		m_IsRunning(false)
	{
		if (!delayed_start)
		{
			Start();
		}
	}

	Restartable::~Restartable()
	{
		Stop();
	}

	void Restartable::Start()
	{
		if (m_IsRunning.load())
		{
			LogTrace(Channel::Devices, "Attempted to start watchdog that is already running; ignoring start request");
		}
		else
		{
			m_IsRunning.store(true);

			// Note the order here....if m_Running is _NOT_ true, Kick() will not action the request...
			
			Kick();
		}
	}

	void Restartable::Kick()
	{
		if (m_IsRunning.load())
		{
			m_WatchdogTimer.expires_from_now(m_TimeoutDuration);
			m_WatchdogTimer.async_wait
			(
				[this](const boost::system::error_code& ec)
				{
					switch (ec.value())
					{
					case boost::system::errc::success:
						// There's not been a message with the <timeout duration> so mark this device as not operating...
						LogWarning(Channel::Devices, "Device timeout: device watchdog has expired");
						m_IsRunning.store(false);
						WatchdogTimeoutOccurred();
						break;

					case boost::asio::error::operation_aborted:
						// IGNORE THIS RESPONSE...  When expires_after() sets the expiry time, any pending asynchronous wait operations will be cancelled and the 
						// handler for each cancelled operation will be invoked with the boost::asio::error::operation_aborted error code.
						break;

					default:
						LogDebug(Channel::Devices, std::format("Device timeout; timer async_wait failed; error -> {}, message -> {}", ec.value(), ec.message()));
						break;
					}
					
				}
			);
		}
		else
		{
			LogTrace(Channel::Devices, "Attempted to kick watchdog that is stopped; ignoring kick request");
		}
	}

	void Restartable::Stop()
	{
		m_IsRunning.store(false);
		m_WatchdogTimer.cancel();
	}

	std::chrono::seconds Restartable::GetTimeout() const
	{
		return m_TimeoutDuration;
	}

	bool Restartable::IsRunning() const
	{
		return m_IsRunning.load();
	}

}
// namespace AqualinkAutomate::Devices::Capabilities
