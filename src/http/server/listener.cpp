#include <format>

#include <boost/asio.hpp>
#include <boost/cobalt.hpp>
#include <boost/system/error_code.hpp>
#include <tl/expected.hpp>

#include "coroutines/cancellation_signal.h"
#include "formatters/asio_endpoint_formatter.h"
#include "http/server/detectsession.h"
#include "http/server/listener.h"
#include "http/server/server_types.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	namespace
	{
		Coroutines::MultiSlot_CancellationSignal detectsession_cancellationsignal;
	}
	// namespace

	boost::cobalt::promise<void> Listener(boost::asio::ip::tcp::endpoint endpoint, std::optional<std::reference_wrapper<boost::asio::ssl::context>> ssl_context)
	{
		auto exec = co_await boost::cobalt::this_coro::executor;

		TcpAcceptor acceptor(exec);
		
		boost::system::error_code ec;
				
		if (acceptor.open(endpoint.protocol(), ec); ec)
		{
			LogWarning(Channel::Web, std::format("Failed to open HTTP server acceptor; error was -> {}", ec.message()));
		}
		else if (acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec); ec)
		{
			LogWarning(Channel::Web, std::format("Failed to configure HTTP server acceptor to reuse address; error was -> {}", ec.message()));
		}
		else if (acceptor.bind(endpoint, ec); ec)
		{
			LogWarning(Channel::Web, std::format("Failed to bind HTTP server to endpoint {}; error was -> {}", endpoint, ec.message()));
		}
		else if (acceptor.listen(boost::asio::socket_base::max_listen_connections, ec); ec)
		{
			LogWarning(Channel::Web, std::format("Failed to listen to HTTP server endpoint {}; error was -> {}", endpoint, ec.message()));
		}
		else
		{
			while (!co_await boost::cobalt::this_coro::cancelled)
			{
				auto [ec, sock] = co_await acceptor.async_accept(boost::asio::as_tuple(boost::cobalt::use_op));
				if (ec)
				{
					LogDebug(Channel::Web, std::format("Accepter has terminated -> {}: {}", ec.value(), ec.message()));
					break;
				}
				else
				{
					// Create a detached coroutine for this session...and go back to listening.
					DetectSession(TcpStream(std::move(sock)), ssl_context, detectsession_cancellationsignal.Slot());
				}
			}

			// Trigger cancellation for any detached sessions (spawned from the co_await DetectSession...).
			detectsession_cancellationsignal.Emit(boost::asio::cancellation_type::all);
		}

		co_return;
	}

}
// namespace AqualinkAutomate::HTTP
