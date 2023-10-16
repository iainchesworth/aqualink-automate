#pragma once 

#include <functional>
#include <memory>
#include <optional>
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
		Listener(Types::ExecutorType executor, boost::asio::ip::tcp::endpoint endpoint);
        Listener(Types::ExecutorType executor, boost::asio::ip::tcp::endpoint endpoint, boost::asio::ssl::context& ssl_context);

	private:
		Listener(Types::ExecutorType executor, boost::asio::ip::tcp::endpoint endpoint, std::optional<std::reference_wrapper<boost::asio::ssl::context>> ssl_context_ref);

	public:
		void Run();

	private:
		void DoAccept();

	private:
		std::optional<std::reference_wrapper<boost::asio::ssl::context>> m_SSLContext;
		boost::asio::ip::tcp::acceptor m_Acceptor;
	};

}
// namespace AqualinkAutomate::HTTP
