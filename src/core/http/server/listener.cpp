#include <deque>
#include <format>

#include <boost/asio.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/system/error_code.hpp>
#include <tl/expected.hpp>
#include "formatters/asio_endpoint_formatter.h"
#include "http/server/detectsession.h"
#include "http/server/listener.h"
#include "http/server/server_types.h"
#include "logging/logging.h"
#include "profiling/factories/profiler_factory.h"
#include "profiling/factories/profiling_unit_factory.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	boost::cobalt::task<void> Listener(boost::asio::ip::tcp::endpoint endpoint, std::optional<std::reference_wrapper<boost::asio::ssl::context>> ssl_context)
	{
		std::deque<boost::asio::cancellation_signal> session_signals;
		auto exec = co_await boost::cobalt::this_coro::executor;

		TcpAcceptor acceptor(exec);
		
		boost::system::error_code ec;
				
		if (acceptor.open(endpoint.protocol(), ec); ec)
		{
			LogWarning(Channel::Web, std::format("Failed to open HTTP server acceptor; error was -> {}", ec.message()));
			LogTrace(Channel::Coroutines, "FAILED: HTTP::Listener could not open acceptor");
		}
		else if (acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec); ec)
		{
			LogWarning(Channel::Web, std::format("Failed to configure HTTP server acceptor to reuse address; error was -> {}", ec.message()));
			LogTrace(Channel::Coroutines, "FAILED: HTTP::Listener could not set socket options");
		}
		else if (acceptor.bind(endpoint, ec); ec)
		{
			LogWarning(Channel::Web, std::format("Failed to bind HTTP server to endpoint {}; error was -> {}", endpoint, ec.message()));
			LogTrace(Channel::Coroutines, "FAILED: HTTP::Listener could not bind to endpoint");
		}
		else if (acceptor.listen(boost::asio::socket_base::max_listen_connections, ec); ec)
		{
			LogWarning(Channel::Web, std::format("Failed to listen to HTTP server endpoint {}; error was -> {}", endpoint, ec.message()));
			LogTrace(Channel::Coroutines, "FAILED: HTTP::Listener could not listen on endpoint");
		}
		else
		{
			LogInfo(Channel::Web, std::format("HTTP listener accepting connections on {}", endpoint));
			LogTrace(Channel::Coroutines, "START: HTTP::Listener");

			while (!co_await boost::cobalt::this_coro::cancelled)
			{
				auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Listener::on_accept", std::source_location::current());

				auto [ec, sock] = co_await acceptor.async_accept(boost::asio::as_tuple(boost::cobalt::use_op));
				if (ec)
				{
					LogDebug(Channel::Web, std::format("Accepter has terminated -> {}: {}", ec.value(), ec.message()));
					LogTrace(Channel::Coroutines, "TERMINATED: HTTP::Listener accept loop ended due to error");
					break;
				}
				else
				{
					LogInfo(Channel::Web, std::format("Accepted new connection from {}", sock.remote_endpoint()));

					// Create a detached coroutine for this session...and go back to listening.
					auto& signal = session_signals.emplace_back();
					Factory::ProfilerFactory::Instance().Get()->Message("New HTTP connection accepted");
					DetectSession(TcpStream(std::move(sock)), ssl_context, signal.slot());
				}
			}

			// Trigger cancellation for any detached sessions (spawned from the co_await DetectSession...).
			for (auto& signal : session_signals)
			{
				signal.emit(boost::asio::cancellation_type::all);
			}
			session_signals.clear();

			LogTrace(Channel::Coroutines, "END: HTTP::HandleHttpSession");
		}

		LogTrace(Channel::Coroutines, "STOP: HTTP::Listener");

		co_return;
	}

}
// namespace AqualinkAutomate::HTTP
