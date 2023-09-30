#pragma once

#include <memory>
#include <string_view>
#include <tuple>
#include <unordered_map>

#include <boost/url.hpp>

#include "http/server/server_types.h"
#include "interfaces/iwebroute.h"
#include "interfaces/iwebsocket.h"

template<> 
struct std::hash<std::tuple<boost::beast::http::verb, boost::urls::url_view>>
{
    std::size_t operator()(const std::tuple<boost::beast::http::verb, boost::urls::url_view>& tuple) const
    {
        std::size_t hash = 0;

        hash_combine(hash, std::hash<int>{}(static_cast<int>(std::get<0>(tuple))));
        hash_combine(hash, std::hash<std::string_view>{}(std::get<1>(tuple).path()));

        return hash;
    }

    template<typename T> 
    void hash_combine(std::size_t& seed, const T& val) const
    {
        std::hash<T> hasher;
        seed ^= hasher(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
};

namespace AqualinkAutomate::HTTP
{

    class RouterOld
    {
    public:
        RouterOld();

    public:
        void Add(Verbs verb, std::unique_ptr<Interfaces::IWebRouteBase>&& handler);
        void Add(std::unique_ptr<Interfaces::IWebSocketBase>&& handler);

    public:
        HTTP::Message HTTP_OnRequest(HTTP::Request const &req);

    public:
        void WebSocket_OnOpen();
        void WebSocket_OnMessage();
        void WebSocket_OnClose();
        void WebSocket_OnError();

    private:
        std::unordered_map<std::tuple<Verbs, boost::urls::url_view>, std::unique_ptr<Interfaces::IWebRouteBase>> m_HttpRoutes;
        std::unordered_map<boost::urls::url_view, std::unique_ptr<Interfaces::IWebSocketBase>> m_WsRoutes;
    };

}
// namespace AqualinkAutomate::HTTP
