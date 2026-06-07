#include <boost/system/error_code.hpp>

#include "devices/capabilities/restartable.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices::Capabilities
{
	const std::chrono::seconds Restartable::DEFAULT_WATCHDOG_TIMEOUT{ std::chrono::seconds(30) };

	Restartable::Restartable(std::chrono::seconds timeout_in_seconds, bool delayed_start) :
		m_TimeoutDuration(timeout_in_seconds),
		//m_WatchdogTimer(co_await boost::cobalt::this_coro::executor),
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
			/*m_WatchdogTimer.expires_from_now(m_TimeoutDuration);
			m_WatchdogTimer.async_wait
			(
				[this](const boost::system::error_code& ec)
				{
					switch (ec.value())
					{
					case boost::system::errc::success:
						// There's not been a message with the <timeout duration> so mark this device as not operating...
						LogWarning(Channel::Devices, "Device timeout: device watchdog has expired");
						m_IsRunning = false;
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
			);*/
		}
		else
		{
			LogTrace(Channel::Devices, "Attempted to kick watchdog that is stopped; ignoring kick request");
		}
	}

	void Restartable::Stop()
	{
		try
		{
			m_IsRunning = false;
			//m_WatchdogTimer.cancel();
		}
		catch (const boost::system::system_error& ex_bse)
		{
			LogDebug(Channel::Devices, std::format("Exception thrown while attempting to cancel watchdog timer; error was -> {}", ex_bse.what()));
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

}
// namespace AqualinkAutomate::Devices::Capabilities
