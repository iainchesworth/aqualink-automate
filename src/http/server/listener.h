#pragma once 

#include <memory>
#include <string>

#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>

#include "http/server/http_session.h"
#include "types/asynchronous_executor.h"

namespace AqualinkAutomate::HTTP
{

	class Listener : public std::enable_shared_from_this<Listener>
	{
	public:
        Listener(Types::ExecutorType executor, boost::asio::ip::tcp::endpoint endpoint, boost::asio::ssl::context& ssl_context);

	public:
		void Run();

	private:
		void DoAccept();

	private:
		boost::asio::ssl::context& m_SSLContext;
		boost::asio::ip::tcp::acceptor m_Acceptor;
	};

}
// namespace AqualinkAutomate::HTTP
