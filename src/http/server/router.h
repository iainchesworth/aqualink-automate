#pragma once

#include <memory>
#include <optional>

#include "http/server/server_types.h"
#include "http/server/static_file_handler.h"
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
        void StaticHandler(std::unique_ptr<StaticFileHandler>&& sfh);

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

    private:
        std::unique_ptr<StaticFileHandler> m_SFH;
    };

}
// namespace AqualinkAutomate::HTTP
