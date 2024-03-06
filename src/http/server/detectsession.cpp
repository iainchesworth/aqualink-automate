#include <chrono>
#include <format>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/cobalt.hpp>

#include "http/server/detectsession.h"
#include "http/server/http_session.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	boost::cobalt::detached DetectSession(TcpStream stream, std::optional<std::reference_wrapper<boost::asio::ssl::context>> ssl_context, boost::asio::cancellation_slot cancellation_slot)
	{
		co_await boost::cobalt::this_coro::reset_cancellation_source(cancellation_slot);

		stream.expires_after(std::chrono::seconds(30));

		boost::beast::flat_buffer buffer;
		
		auto ssl_was_detected = co_await boost::beast::async_detect_ssl(stream, buffer);

		if (ssl_was_detected && ssl_context.has_value())
		{
			LogTrace(Channel::Web, "SSL detected on HTTP stream -> transitioning to HTTPS");
			SslStream ssl_stream{ std::move(stream), ssl_context.value().get() };
			auto [ec, bytes_used] = co_await ssl_stream.async_handshake(boost::asio::ssl::stream_base::server, buffer.data(), boost::asio::as_tuple(boost::cobalt::use_op));
			if (ec)
			{
				LogDebug(Channel::Web, std::format("Failed to complete SSL handshake on HTTPS stream; error was -> {}", ec.message()));
			}
			else
			{
				LogTrace(Channel::Web, std::format("Completed SSL handshake on HTTPS stream; {} bytes were consumed", bytes_used));
				
				buffer.consume(bytes_used);

				co_await HandleHttpSession(ssl_stream, buffer);

			}
			
		}
		else if (ssl_was_detected)
		{
			LogInfo(Channel::Web, "SSL detected on HTTP stream but HTTPS is disabled -> ignoring...");
		}
		else
		{
			LogTrace(Channel::Web, "SSL not detected on HTTP stream -> staying with HTTP");
			co_await HandleHttpSession(stream, buffer);
		}

		co_return;
	}

}
// namespace AqualinkAutomate::HTTP
