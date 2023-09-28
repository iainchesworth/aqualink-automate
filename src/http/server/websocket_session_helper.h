#pragma once

#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include "http/server/websocket_plainsession.h"
#include "http/server/websocket_sslsession.h"

namespace AqualinkAutomate::HTTP
{

    template<class BODY, class ALLOCATOR>
    void WebSocket_MakeSession(boost::beast::tcp_stream stream, boost::beast::http::request<BODY, boost::beast::http::basic_fields<ALLOCATOR>> req)
    {
        std::make_shared<WebSocket_PlainSession>(std::move(stream))->Run(std::move(req));
    }

    template<class BODY, class ALLOCATOR>
    void WebSocket_MakeSession(boost::beast::ssl_stream<boost::beast::tcp_stream> stream, boost::beast::http::request<BODY, boost::beast::http::basic_fields<ALLOCATOR>> req)
    {
        std::make_shared<WebSocket_SSLSession>(std::move(stream))->Run(std::move(req));
    }

}
// namespace AqualinkAutomate::HTTP
