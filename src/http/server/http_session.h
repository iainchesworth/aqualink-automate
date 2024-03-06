#pragma once

#include <format>
#include <memory>
#include <tuple>
#include <typeinfo>

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/cobalt/join.hpp>
#include <boost/cobalt/promise.hpp>
#include <boost/system/error_code.hpp>

#include "http/server/websocket_session.h"
#include "http/server/server_types.h"
#include "logging/logging.h"

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
			co_await CloseStream(stream);			
		}

		if (ec)
		{
			LogTrace(Channel::Web, std::format("Failed during read of HTTP stream; error was -> {}", ec.message()));
		}
		else
		{
			while (!co_await boost::cobalt::this_coro::cancelled)
			{
				if (boost::beast::websocket::is_upgrade(parser->get()))
				{
					LogTrace(Channel::Web, "WebSocket upgrade requested detected on HTTP stream -> transitioning to WebSockets");
					boost::beast::get_lowest_layer(stream).expires_never();
					
					co_await HandleWebSocketSession(stream, buffer, parser->release());
					break;
				}
				else
				{
					LogTrace(Channel::Web, "HTTP request from client; handling request...");

					auto msg = Routing::HTTP_OnRequest(parser->release());
					if (!msg.keep_alive())
					{
						auto [ec, _] = co_await boost::beast::async_write(stream, std::move(msg), boost::asio::as_tuple(boost::cobalt::use_op));
						if (ec)
						{
							break;
						}
					}
					else
					{
						parser.reset();
						parser.emplace();
						parser->body_limit(10000);

						auto [read_op, write_op] = co_await boost::cobalt::join(
							boost::beast::http::async_read(stream, buffer, *parser, boost::asio::as_tuple(boost::cobalt::use_op)),
							boost::beast::async_write(stream, std::move(msg), boost::asio::as_tuple(boost::cobalt::use_op))
						);

						auto [ec_r, sz_r] = read_op;
						auto [ec_w, sz_w] = write_op;

						LogTrace(Channel::Web, std::format("HTTP session read_op completed ({} bytes read) with the following -> {}: {}", sz_r, ec_r.value(), ec_r.message()));
						LogTrace(Channel::Web, std::format("HTTP session write_op completed ({} bytes written) with the following -> {}: {}", sz_w, ec_w.value(), ec_w.message()));

						if (ec_r || ec_w)
						{
							LogDebug(Channel::Web, std::format("HTTP session -> terminating"));
							break;
						}
					}
				}
			}
		}

		co_return;
	}

}
// namespace AqualinkAutomate::HTTP
