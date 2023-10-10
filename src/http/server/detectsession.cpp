#include <chrono>
#include <format>

#include <boost/asio/basic_socket_acceptor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/beast/core/detect_ssl.hpp>
#include <boost/system/error_code.hpp>

#include "http/server/detectsession.h"
#include "http/server/http_plainsession.h"
#include "http/server/http_sslsession.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	DetectSession::DetectSession(boost::asio::ip::tcp::socket&& socket, boost::asio::ssl::context& ssl_context) :
		m_Stream(std::move(socket)),
		m_SSLContext(ssl_context)
	{
	}

	void DetectSession::Run()
	{
		boost::asio::dispatch(
			m_Stream.get_executor(),
			[this, self = shared_from_this()]() -> void
			{
				m_Stream.expires_after(std::chrono::seconds(30));

				boost::beast::async_detect_ssl(
					m_Stream,
					m_Buffer,
					[this, self = shared_from_this()](boost::system::error_code ec, bool ssl_was_detected)
					{
						if (ec)
						{
							LogDebug(Channel::Web, std::format("Failed to complete SSL detection on HTTP stream; error was -> {}", ec.message()));
						}
						else if (ssl_was_detected)
						{
							LogTrace(Channel::Web, "SSL detected on HTTP stream -> transitioning to HTTPS");
							std::make_shared<HTTP_SSLSession>(std::move(m_Stream), m_SSLContext, std::move(m_Buffer))->Run();
						}
						else
						{
							LogTrace(Channel::Web, "SSL not detected on HTTP stream -> staying with HTTP");
                            std::make_shared<HTTP_PlainSession>(std::move(m_Stream), std::move(m_Buffer))->Run();
						}
					}
				);
			}
		);
	}

}
// namespace AqualinkAutomate::HTTP
