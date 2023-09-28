#include <chrono>

#include <boost/system/error_code.hpp>

#include "http/server/http_sslsession.h"

namespace AqualinkAutomate::HTTP
{

	HTTP_SSLSession::HTTP_SSLSession(boost::beast::tcp_stream&& stream, boost::asio::ssl::context& ssl_context, boost::beast::flat_buffer&& buffer, std::shared_ptr<Router> router) :
		HTTP_Session<HTTP_SSLSession>(std::move(buffer), router),
		m_Stream(std::move(stream), ssl_context)
	{
	}

	void HTTP_SSLSession::Run()
	{
		boost::beast::get_lowest_layer(m_Stream).expires_after(std::chrono::seconds(30));

		m_Stream.async_handshake(
			boost::asio::ssl::stream_base::server,
			m_Buffer.data(),
                                 [this, self = shared_from_this()](boost::system::error_code ec, std::size_t bytes_used)
			{
				switch (ec.value())
				{
                case boost::system::errc::success:
					m_Buffer.consume(bytes_used);
					DoRead();
					break;

				default:
					break;
				}
			}
		);
	}

	boost::beast::ssl_stream<boost::beast::tcp_stream>& HTTP_SSLSession::Stream()
	{
		return m_Stream;
	}

	boost::beast::ssl_stream<boost::beast::tcp_stream> HTTP_SSLSession::ReleaseStream()
	{
		return std::move(m_Stream);
	}

	void HTTP_SSLSession::DoEOF()
	{
		boost::beast::get_lowest_layer(m_Stream).expires_after(std::chrono::seconds(30));

		m_Stream.async_shutdown(
            [this, self = shared_from_this()](boost::system::error_code ec)
			{
                switch (ec.value())
				{
                case boost::system::errc::success:
					// Nothing...
					break;

				default:
					break;
				}
			}
		);
	}

}
// AqualinkAutomate::HTTP
