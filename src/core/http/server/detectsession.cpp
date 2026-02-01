#include <chrono>
#include <format>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "http/server/detectsession.h"
#include "http/server/http_session.h"
#include "logging/logging.h"
#include "profiling/factories/profiler_factory.h"
#include "profiling/factories/profiling_unit_factory.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	boost::cobalt::detached DetectSession(TcpStream stream, std::optional<std::reference_wrapper<boost::asio::ssl::context>> ssl_context, boost::asio::cancellation_slot cancellation_slot)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("DetectSession", std::source_location::current());

		try
		{
			co_await boost::cobalt::this_coro::reset_cancellation_source(cancellation_slot);

			stream.expires_after(std::chrono::seconds(30));

			boost::beast::flat_buffer buffer;

			auto [ads_ec, ssl_was_detected] = co_await boost::beast::async_detect_ssl(stream, buffer, boost::asio::as_tuple(boost::cobalt::use_op));
			if (ads_ec)
			{
				LogDebug(Channel::Web, std::format("Failed to perform SSL detection; error was {} -> {}", ads_ec.value(), ads_ec.message()));
			}
			else if (ssl_was_detected && ssl_context.has_value())
			{
				zone->Text("HTTPS");
				LogTrace(Channel::Web, "SSL detected on HTTP stream -> transitioning to HTTPS");
				SslStream ssl_stream{ std::move(stream), ssl_context.value().get() };
				auto [ah_ec, bytes_used] = co_await ssl_stream.async_handshake(boost::asio::ssl::stream_base::server, buffer.data(), boost::asio::as_tuple(boost::cobalt::use_op));
				if (ah_ec)
				{
					LogDebug(Channel::Web, std::format("Failed to complete SSL handshake on HTTPS stream; error was {} -> {}", ah_ec.value(), ah_ec.message()));
				}
				else
				{
					LogTrace(Channel::Web, std::format("Completed SSL handshake on HTTPS stream; {} bytes were consumed", bytes_used));

					buffer.consume(bytes_used);

					// Store the promise in a named variable to ensure its lifetime
					// extends across the co_await suspension point. Prevents potential
					// issues with MSVC coroutine temporary lifetime handling.
					auto session = HandleHttpSession(ssl_stream, buffer);
					co_await session;
				}
			}
			else if (ssl_was_detected)
			{
				LogInfo(Channel::Web, "SSL detected on HTTP stream but HTTPS is disabled -> ignoring...");
			}
			else
			{
				zone->Text("HTTP");
				LogTrace(Channel::Web, "SSL not detected on HTTP stream -> staying with HTTP");

				auto session = HandleHttpSession(stream, buffer);
				co_await session;
			}
		}
		catch (const std::exception& ex)
		{
			LogWarning(Channel::Web, std::format("DetectSession terminated with exception: {}", ex.what()));
		}
		catch (...)
		{
			LogWarning(Channel::Web, "DetectSession terminated with unknown exception");
		}

		co_return;
	}

}
// namespace AqualinkAutomate::HTTP
