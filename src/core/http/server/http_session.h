#pragma once

#include <format>
#include <memory>
#include <tuple>
#include <typeinfo>

#include <boost/asio/as_tuple.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/cobalt/promise.hpp>
#include <boost/system/error_code.hpp>

#include "http/server/websocket_session.h"
#include "http/server/server_types.h"
#include "logging/logging.h"
#include "profiling/factories/profiler_factory.h"
#include "profiling/factories/profiling_unit_factory.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

	boost::cobalt::promise<void> CloseStream(TcpStream& stream);
	boost::cobalt::promise<void> CloseStream(SslStream& stream);

	template<typename TCP_STREAM>
	boost::cobalt::promise<void> HandleHttpSession(TCP_STREAM& stream, boost::beast::flat_buffer& buffer)
	{
		std::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> parser;

		parser.emplace();
		parser->body_limit(10000);

		boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

		auto [ec, _] = co_await boost::beast::http::async_read(stream, buffer, *parser, boost::asio::as_tuple(boost::cobalt::use_op));
		if (boost::beast::http::error::end_of_stream == ec)
		{
			auto close_op = CloseStream(stream);
			co_await close_op;
		}

		if (ec)
		{
			LogTrace(Channel::Web, std::format("Failed during read of HTTP stream; error was -> {}", ec.message()));
		}
		else
		{
			LogTrace(Channel::Coroutines, "START: HTTP::HandleHttpSession");

			while (!co_await boost::cobalt::this_coro::cancelled)
			{
				auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("HTTP Request", std::source_location::current());

				if (boost::beast::websocket::is_upgrade(parser->get()))
				{
					zone->Text("WebSocket Upgrade");
					LogTrace(Channel::Web, "WebSocket upgrade requested detected on HTTP stream -> transitioning to WebSockets");
					boost::beast::get_lowest_layer(stream).expires_never();

					auto ws_session = HandleWebSocketSession(stream, buffer, parser->release());
					co_await ws_session;
					break;
				}
				else
				{
					zone->Text(std::string(parser->get().target()));
					LogTrace(Channel::Web, "HTTP request from client; handling request...");

					auto msg = Routing::HTTP_OnRequest(parser->release());
					bool wants_keep_alive = msg.keep_alive();

					auto [ec_w, sz_w] = co_await boost::beast::async_write(stream, std::move(msg), boost::asio::as_tuple(boost::cobalt::use_op));
					if (ec_w)
					{
						LogDebug(Channel::Web, std::format("HTTP session write failed ({} bytes written) -> {}: {}", sz_w, ec_w.value(), ec_w.message()));
						break;
					}

					LogTrace(Channel::Web, std::format("HTTP session write completed ({} bytes written)", sz_w));

					if (!wants_keep_alive)
					{
						auto close_op = CloseStream(stream);
			co_await close_op;
						break;
					}

					parser.reset();
					parser.emplace();
					parser->body_limit(10000);

					boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

					auto [ec_r, sz_r] = co_await boost::beast::http::async_read(stream, buffer, *parser, boost::asio::as_tuple(boost::cobalt::use_op));
					if (ec_r)
					{
						LogTrace(Channel::Web, std::format("HTTP session read failed ({} bytes read) -> {}: {}", sz_r, ec_r.value(), ec_r.message()));
						break;
					}

					LogTrace(Channel::Web, std::format("HTTP session read completed ({} bytes read)", sz_r));
				}

				LogTrace(Channel::Coroutines, "END: HTTP::HandleHttpSession");
			}
		}

		co_return;
	}

}
// namespace AqualinkAutomate::HTTP
