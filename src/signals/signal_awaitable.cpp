#include <exception>
#include <format>

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/system/error_code.hpp>

#include "logging/logging.h"
#include "signals/signal_awaitable.h"
#include "signals/signal_handler.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Signals
{
	void Signal_Awaitable(boost::asio::signal_set& signals)
	{
		try
		{
			LogDebug(Channel::Signals, "Initialising signal handlers for SIGINT, SIGTERM");

			LogTrace(Channel::Signals, "Adding SIGINT and SIGTERM ids to signal set");
			signals.add(SIGINT);
			signals.add(SIGTERM);

#ifdef SIGQUIT
			LogTrace(Channel::Signals, "Adding SIGQUIT ids to signal set");
			signals.add(SIGQUIT);
#endif // SIGQUIT

			signals.async_wait([&](const boost::system::error_code& ec, int signal_number)
				{
					switch (ec.value())
					{
					case boost::system::errc::success:
						LogTrace(Channel::Signals, "Successfully yield'd -> async_waited() for signal handler");
						SignalHandler(signal_number);
						break;

					case boost::system::errc::operation_canceled:
						LogDebug(Channel::Signals, "Signal handler's async_wait was cancelled.");
						[[fallthrough]];
					default:
						LogError(Channel::Signals, std::format("Error occured in signal handler.  Error Code: {}", ec.value()));
						LogDebug(Channel::Signals, std::format("Error message: {}", ec.message()));
						break;
					}
				}
			);
		}
		catch (const std::exception& ex)
		{
			LogError(Channel::Signals, std::format("Exception occured while attempting to wait for signal handler: message -> {}", ex.what()));
		}
	}
}
// AqualinkAutomate::Signals