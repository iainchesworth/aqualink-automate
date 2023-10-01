#pragma once

#include <memory>

#include <boost/url.hpp>

#include "http/server/server_types.h"
#include "http/server/routing/node.h"
#include "interfaces/iwebroute.h"
#include "interfaces/iwebsocket.h"


namespace AqualinkAutomate::HTTP
{

    class Router
    {
    public:
        Router();

    public:
        void Add(Verbs verb, std::shared_ptr<Interfaces::IWebRouteBase> handler);
        void Add(std::shared_ptr<Interfaces::IWebSocketBase> handler);

    public:
        HTTP::Message HTTP_OnRequest(HTTP::Request const &req);

    public:
        void WebSocket_OnOpen();
        void WebSocket_OnMessage();
        void WebSocket_OnClose();
        void WebSocket_OnError();

    private:
        HTTP::Routing::impl<Interfaces::IWebRouteBase> m_HttpRoutes;
        HTTP::Routing::impl<Interfaces::IWebSocketBase> m_WsRoutes;
    };

}
// namespace AqualinkAutomate::HTTP
