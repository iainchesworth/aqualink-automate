#include <format>

#include <magic_enum.hpp>

#include "exceptions/exception_http_duplicateroute.h"
#include "http/server/response_404.h"
#include "http/server/router.h"

#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

    Router::Router() : 
        m_HttpRoutes(), 
        m_WsRoutes()
    {
    }

    void Router::Add(Verbs verb, std::unique_ptr<Interfaces::IWebRouteBase>&& handler)
    {
        if (auto url = boost::urls::parse_origin_form(handler->Route()); url.has_error())
        {
            throw; ///FIXME
        }
        else if (m_HttpRoutes.contains({verb, url.value()}))
        {
            throw Exceptions::HTTP_DuplicateRoute();
        }
        else
        {
            m_HttpRoutes[{verb, url.value()}] = std::move(handler);
        }
    }

    void Router::Add(std::unique_ptr<Interfaces::IWebSocketBase>&& handler)
    {
        if (auto url = boost::urls::parse_origin_form(handler->Route()); url.has_error())
        {
            throw; /// FIXME
        }
        else if (m_WsRoutes.contains(url.value()))
        {
            throw Exceptions::HTTP_DuplicateRoute();
        }
        else
        {
            m_WsRoutes[url.value()] = std::move(handler);
        }
    }

    HTTP::Message Router::HTTP_OnRequest(HTTP::Request const& req)
    {
        try
        {
            if (auto url = boost::urls::parse_origin_form(req.target()); url.has_error())
            {
                LogDebug(Channel::Web, std::format("Failed to parse HTTP request: {} -> error: {}", req.target().data(), url.error().message()));
            }
            else if (auto stripped_url = boost::urls::parse_origin_form(url->path()); stripped_url.has_error())
            {
                LogDebug(Channel::Web, std::format("Failed to parse stripped URL: {} -> error: {}", url->path(), stripped_url.error().message()));
            }
            else if (!m_HttpRoutes.contains({req.method(), stripped_url.value()}))
            {
                LogDebug(Channel::Web, std::format("Failed to match <method, route> key in route collection -> key was <{}, {}>", magic_enum::enum_name(req.method()), stripped_url.value().path()));

                return HTTP::Response_404(req);
            }
            else
            {
                return m_HttpRoutes[{req.method(), stripped_url.value()}]->OnRequest(req);
            }
        }
        catch (...)
        {
            throw; /// FIXME 5xx
        }

        throw;
    }

}
// namespace AqualinkAutomate::HTTP
