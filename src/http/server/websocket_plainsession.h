#pragma once

#include <memory>

#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/websocket/stream.hpp>

#include "http/server/websocket_session.h"

namespace AqualinkAutomate::HTTP
{

    class WebSocket_PlainSession : public WebSocket_Session<WebSocket_PlainSession>, public std::enable_shared_from_this<WebSocket_PlainSession>
    {
    public:
        explicit WebSocket_PlainSession(boost::beast::tcp_stream&& stream);

    public:
        boost::beast::websocket::stream<boost::beast::tcp_stream>& WS();

    private:
        boost::beast::websocket::stream<boost::beast::tcp_stream> m_WS;
    };

}
// namespace AqualinkAutomate::HTTP
