#include "http/server/websocket_sslsession.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

    WebSocket_SSLSession::WebSocket_SSLSession(boost::beast::ssl_stream<boost::beast::tcp_stream>&& stream) :
        WebSocket_Session<WebSocket_SSLSession>(),
        std::enable_shared_from_this<WebSocket_SSLSession>(),
        m_WS(std::move(stream))
    {
        LogTrace(Channel::Web, std::format("Creating WebSocket SSL session: {}", boost::uuids::to_string(Id())));
    }

    WebSocket_SSLSession::~WebSocket_SSLSession()
    {
        LogTrace(Channel::Web, std::format("Destroying WebSocket SSL session: {}", boost::uuids::to_string(Id())));
    }

    boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>& WebSocket_SSLSession::WS()
    {
        return m_WS;
    }

}
// namespace AqualinkAutomate::HTTP
