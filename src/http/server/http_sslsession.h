#pragma once

#include <memory>

#include <boost/asio/ssl/context.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/core/tcp_stream.hpp>

#include "http/server/http_session.h"

namespace AqualinkAutomate::HTTP
{

	class HTTP_SSLSession : public HTTP_Session<HTTP_SSLSession>, public std::enable_shared_from_this<HTTP_SSLSession>
	{
	public:
        HTTP_SSLSession(boost::beast::tcp_stream&& stream, boost::asio::ssl::context& ssl_context, boost::beast::flat_buffer&& buffer);
		virtual ~HTTP_SSLSession();

	public:
		void Run();

	public:
		boost::beast::ssl_stream<boost::beast::tcp_stream>& Stream();
		boost::beast::ssl_stream<boost::beast::tcp_stream> ReleaseStream();

	public:
		void DoEOF();

	private:
		boost::beast::ssl_stream<boost::beast::tcp_stream> m_Stream;
	};

}
// AqualinkAutomate::HTTP
