#include "http/server/websocket_sslsession.h"

namespace AqualinkAutomate::HTTP
{

    WebSocket_SSLSession::WebSocket_SSLSession(boost::beast::ssl_stream<boost::beast::tcp_stream>&& stream) :
        m_WS(std::move(stream))
    {
    }

    boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>& WebSocket_SSLSession::WS()
    {
        return m_WS;
    }

}
// namespace AqualinkAutomate::HTTP
