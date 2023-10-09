#pragma once

#include <memory>
#include <string>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>

#include "http/server/router.h"

namespace AqualinkAutomate::HTTP
{

	class DetectSession : public std::enable_shared_from_this<DetectSession>
	{
	public:
        explicit DetectSession(boost::asio::ip::tcp::socket &&socket, boost::asio::ssl::context &ssl_context, std::shared_ptr<Router> router);

	public:
		void Run();

	private:
		boost::beast::tcp_stream m_Stream;
		boost::asio::ssl::context& m_SSLContext;
		std::shared_ptr<Router> m_Router;
		boost::beast::flat_buffer m_Buffer;
	};

}
// namespace AqualinkAutomate::HTTP