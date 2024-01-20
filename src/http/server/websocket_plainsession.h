#pragma once

#include <memory>

#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/websocket/stream.hpp>

#include "http/server/websocket_session.h"

namespace AqualinkAutomate::HTTP
{

    template<typename TCP_STREAM>
    class WebSocket_PlainSession_Base : public WebSocket_Session<WebSocket_PlainSession_Base<TCP_STREAM>>, public std::enable_shared_from_this<WebSocket_PlainSession_Base<TCP_STREAM>>
    {
    public:
        explicit WebSocket_PlainSession_Base(TCP_STREAM&& stream) :
            m_WS(std::move(stream))
        {
        }

    public:
        boost::beast::websocket::stream<TCP_STREAM>& WS()
        {
            return m_WS;
        }

    private:
        boost::beast::websocket::stream<TCP_STREAM> m_WS;
    };

    using WebSocket_PlainSession = WebSocket_PlainSession_Base<boost::beast::tcp_stream>;

}
// namespace AqualinkAutomate::HTTP
