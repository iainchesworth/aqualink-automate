#include <chrono>
#include <format>

#include <boost/system/error_code.hpp>

#include "http/server/http_sslsession.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	HTTP_SSLSession::HTTP_SSLSession(boost::beast::tcp_stream&& stream, boost::asio::ssl::context& ssl_context, boost::beast::flat_buffer&& buffer) :
		HTTP_Session<HTTP_SSLSession>(std::move(buffer)),
		std::enable_shared_from_this<HTTP_SSLSession>(),
		m_Stream(std::move(stream), ssl_context)
	{
		LogTrace(Channel::Web, std::format("Creating HTTP SSL session: {}", boost::uuids::to_string(Id())));
	}

	HTTP_SSLSession::~HTTP_SSLSession()
	{
		LogTrace(Channel::Web, std::format("Destroying HTTP SSL session: {}", boost::uuids::to_string(Id())));
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
					LogTrace(Channel::Web, std::format("Completed SSL handshake on HTTPS stream; {} bytes were consumed", bytes_used));
					m_Buffer.consume(bytes_used);
					DoRead();
					break;

				default:
					LogDebug(Channel::Web, std::format("Failed to complete SSL handshake on HTTPS stream; error was -> {}", ec.message()));
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
					LogTrace(Channel::Web, "Completed SSL shutdown on HTTPS stream");
					break;

				default:
					LogDebug(Channel::Web, std::format("Failed to complete SSL shutdown on HTTPS stream; error was -> {}", ec.message()));
					break;
				}
			}
		);
	}

}
// AqualinkAutomate::HTTP
