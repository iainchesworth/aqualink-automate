#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>

#include "http/server/detectsession.h"
#include "http/server/listener.h"

namespace AqualinkAutomate::HTTP
{

	Listener::Listener(Types::ExecutorType executor, boost::asio::ip::tcp::endpoint endpoint, boost::asio::ssl::context &ssl_context, std::shared_ptr<Router> router) :
		m_SSLContext(ssl_context),
		m_Acceptor(boost::asio::make_strand(executor)),
		m_Router(router)
	{
        boost::system::error_code ec;

		if (m_Acceptor.open(endpoint.protocol(), ec); ec)
		{
			throw;
		}
		else if (m_Acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec); ec)
		{
			throw;
		}
		else if (m_Acceptor.bind(endpoint, ec); ec)
		{
			throw;
		}
		else if (m_Acceptor.listen(boost::asio::socket_base::max_listen_connections, ec); ec)
		{
			throw;
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
					std::make_shared<DetectSession>(std::move(socket), m_SSLContext, m_Router)->Run();
					break;

				default:
					break;
				}

				DoAccept();
			}
		);
	}

}
// namespace AqualinkAutomate::HTTP
