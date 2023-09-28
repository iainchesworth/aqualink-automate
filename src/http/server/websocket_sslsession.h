#pragma once

#include <memory>

#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/websocket/stream.hpp>

#include "http/server/websocket_session.h"

namespace AqualinkAutomate::HTTP
{

    class WebSocket_SSLSession : public WebSocket_Session<WebSocket_SSLSession>, public std::enable_shared_from_this<WebSocket_SSLSession>
    {
    public:
        explicit WebSocket_SSLSession(boost::beast::ssl_stream<boost::beast::tcp_stream>&& stream);

    public:
        boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>& WS();

    private:
        boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>> m_WS;
    };

}
// namespace AqualinkAutomate::HTTP
