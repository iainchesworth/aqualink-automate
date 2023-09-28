#include "http/server/websocket_plainsession.h"

namespace AqualinkAutomate::HTTP
{

    WebSocket_PlainSession::WebSocket_PlainSession(boost::beast::tcp_stream&& stream) :
        m_WS(std::move(stream))
    {
    }

    boost::beast::websocket::stream<boost::beast::tcp_stream>& WebSocket_PlainSession::WS()
    {
        return m_WS;
    }

}
// namespace AqualinkAutomate::HTTP
