#pragma once

#include <memory>

#include <boost/asio/as_tuple.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/cobalt/detached.hpp>
#include <boost/cobalt/promise.hpp>
#include <boost/cobalt/join.hpp>

#include "http/server/routing/routing.h"
#include "http/server/server_types.h"
#include "interfaces/iwebsocket.h"
#include "logging/logging.h"
#include "profiling/factories/profiler_factory.h"
#include "profiling/factories/profiling_unit_factory.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

    template<typename WEBSOCKET_STREAM>
    boost::cobalt::promise<void> HandleWebSocketSession_ReadOp(WEBSOCKET_STREAM& ws, boost::beast::flat_buffer& buffer, Interfaces::IWebSocketBase* ws_route_handler)
    {
        LogTrace(Channel::Coroutines, "START: HTTP::HandleWebSocketSession_ReadOp");

        while (!co_await boost::cobalt::this_coro::cancelled)
        {
            auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WS Read", std::source_location::current());

            auto [ec, bytes_transferred] = co_await ws.async_read(buffer, boost::asio::as_tuple(boost::cobalt::use_op));
            if (boost::beast::websocket::error::closed == ec)
            {
                LogTrace(Channel::Web, "WebSocket was closed (during read); calling OnClose handler");
                ws_route_handler->OnClose();
                break;
            }
            else if (ec)
            {
                LogTrace(Channel::Web, std::format("Failed during read of WebSocket stream; error was -> {}", ec.message()));
                ws_route_handler->OnError();
                break;
            }
            else
            {
                zone->Value(bytes_transferred);
                ws_route_handler->OnMessage(buffer);
            }
        }

        LogTrace(Channel::Coroutines, "END: HTTP::HandleWebSocketSession_ReadOp");

        co_return;
    }

    template<typename WEBSOCKET_STREAM>
    boost::cobalt::promise<void> HandleWebSocketSession_WriteOp(WEBSOCKET_STREAM& ws, Interfaces::IWebSocketBase* ws_route_handler)
    {
        auto msg_generator = ws_route_handler->MessageGenerator();

        LogTrace(Channel::Coroutines, "START: HTTP::HandleWebSocketSession_WriteOp");

        while (!co_await boost::cobalt::this_coro::cancelled)
        {
            auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WS Write", std::source_location::current());

            auto msg = co_await msg_generator;

            ws.binary(false);

            auto [ec, bytes_transferred] = co_await ws.async_write(boost::asio::buffer(msg), boost::asio::as_tuple(boost::cobalt::use_op));
            if (boost::beast::websocket::error::closed == ec)
            {
                LogTrace(Channel::Web, "WebSocket was closed (during write); calling OnClose handler");
                ws_route_handler->OnClose();
                break;
            }
            else if (ec)
            {
                LogDebug(Channel::Web, std::format("Failed during write of WebSocket stream; error was -> {}", ec.message()));
                ws_route_handler->OnError();
                break;
            }
            else
            {
                zone->Value(bytes_transferred);
                ws_route_handler->OnPublish();
            }
        }

        msg_generator.cancel();

        LogTrace(Channel::Coroutines, "END: HTTP::HandleWebSocketSession_WriteOp");

        co_return;
    }

    template<typename WEBSOCKET_STREAM, class REQUEST_BODY, class REQUEST_ALLOCATOR>
    boost::cobalt::promise<void> HandleWebSocketSession(WEBSOCKET_STREAM& stream, boost::beast::flat_buffer& buffer, boost::beast::http::request<REQUEST_BODY, boost::beast::http::basic_fields<REQUEST_ALLOCATOR>> req)
    {
        boost::beast::websocket::stream<WEBSOCKET_STREAM&> ws{ stream };

        ws.set_option(
            boost::beast::websocket::stream_base::timeout::suggested(
                boost::beast::role_type::server
            )
        );

        ws.set_option(
            boost::beast::websocket::stream_base::decorator(
                [](boost::beast::websocket::response_type& res)
                {
                    res.set(
                        boost::beast::http::field::server,
                        std::string(BOOST_BEAST_VERSION_STRING) + " advanced-server-flex" ///FIXME
                    );
                }
            )
        );

        auto ws_accept_zone = Factory::ProfilingUnitFactory::Instance().CreateZone("WS Accept", std::source_location::current());
        ws_accept_zone->Text(std::string(req.target()));

        if (auto ws_route_handler = Routing::WS_OnAccept(req.target()); nullptr == ws_route_handler)
        {
            LogDebug(Channel::Web, "Invalid session pointer; no OnOpen handler being called");
        }
        else if (auto [ec] = co_await ws.async_accept(req, boost::asio::as_tuple(boost::cobalt::use_op)); ec)
        {
            LogDebug(Channel::Web, std::format("Failed to accept the websocket connection -> {}: {}", ec.value(), ec.message()));
        }
        else
        {
            ws_route_handler->OnOpen();

            // Execute the websocket reads and writes in parallel; these co-routines will continue
            // until one or more is cancelled or has an error.

            auto read_promise = HandleWebSocketSession_ReadOp(ws, buffer, ws_route_handler);
            auto write_promise = HandleWebSocketSession_WriteOp(ws, ws_route_handler);
            co_await boost::cobalt::join(read_promise, write_promise);
        }

        co_return;
    }

}
// namespace AqualinkAutomate::HTTP
