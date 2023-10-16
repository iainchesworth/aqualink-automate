#include <format>

#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>

#include "formatters/asio_endpoint_formatter.h"
#include "http/server/detectsession.h"
#include "http/server/listener.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{
	Listener::Listener(Types::ExecutorType executor, boost::asio::ip::tcp::endpoint endpoint) :
		Listener(executor, endpoint, std::nullopt)
	{
	}

	Listener::Listener(Types::ExecutorType executor, boost::asio::ip::tcp::endpoint endpoint, boost::asio::ssl::context& ssl_context) :
		Listener(executor, endpoint, std::make_optional(std::ref(ssl_context)))
	{
	}

	Listener::Listener(Types::ExecutorType executor, boost::asio::ip::tcp::endpoint endpoint, std::optional<std::reference_wrapper<boost::asio::ssl::context>> ssl_context_ref) :
		m_SSLContext(ssl_context_ref),
		m_Acceptor(boost::asio::make_strand(executor))
	{
		boost::system::error_code ec;

		if (m_Acceptor.open(endpoint.protocol(), ec); ec)
		{
			LogWarning(Channel::Web, std::format("Failed to open HTTP server acceptor; error was -> {}", ec.message()));
		}
		else if (m_Acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec); ec)
		{
			LogWarning(Channel::Web, std::format("Failed to configure HTTP server acceptor to reuse address; error was -> {}", ec.message()));
		}
		else if (m_Acceptor.bind(endpoint, ec); ec)
		{
			LogWarning(Channel::Web, std::format("Failed to bind HTTP server to endpoint {}; error was -> {}", endpoint, ec.message()));
		}
		else if (m_Acceptor.listen(boost::asio::socket_base::max_listen_connections, ec); ec)
		{
			LogWarning(Channel::Web, std::format("Failed to listen to HTTP server endpoint {}; error was -> {}", endpoint, ec.message()));
		}
		else
		{
			// All good.
		}
	}

	void Listener::Run()
	{
		DoAccept();
	}

	void Listener::DoAccept()
	{
		m_Acceptor.async_accept(
			boost::asio::make_strand(m_Acceptor.get_executor()),
			[this, self = shared_from_this()](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) -> void
			{
				switch (ec.value())
				{
                case boost::system::errc::success:
					std::make_shared<DetectSession>(std::move(socket), m_SSLContext)->Run();
					break;

				default:
					LogDebug(Channel::Web, std::format("Failed to accept HTTP connection from {}; error was -> {}", socket.remote_endpoint(), ec.message()));
					break;
				}

				DoAccept();
			}
		);
	}

}
// namespace AqualinkAutomate::HTTP
