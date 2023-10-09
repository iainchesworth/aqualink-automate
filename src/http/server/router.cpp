#include <format>

#include <boost/url/parse.hpp>
#include <boost/url/parse_path.hpp>
#include <boost/url/url.hpp>
#include <magic_enum.hpp>

#include "exceptions/exception_http_duplicateroute.h"
#include "formatters/beast_stringview_formatter.h"
#include "formatters/url_segments_encoded_view_formatter.h"
#include "http/server/responses/response_staticfile.h"
#include "http/server/responses/response_400.h"
#include "http/server/responses/response_404.h"
#include "http/server/responses/response_500.h"
#include "http/server/router.h"
#include "http/server/routing/matches.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP
{

    Router::Router() : 
        m_HttpRoutes(), 
        m_WsRoutes(),
        m_SFH(nullptr)
    {
    }

    void Router::Add(Verbs verb, std::shared_ptr<Interfaces::IWebRouteBase> handler)
    {
        LogTrace(Channel::Web, std::format("Adding HTTP handler for route '{}'", handler->Route()));
        m_HttpRoutes.insert_impl(handler->Route(), handler);
    }

    void Router::Add(std::shared_ptr<Interfaces::IWebSocketBase> handler)
    {
        LogTrace(Channel::Web, std::format("Adding WebSocket handler for route '{}'", handler->Route()));
        m_WsRoutes.insert_impl(handler->Route(), handler);
    }

    void Router::StaticHandler(std::unique_ptr<StaticFileHandler>&& sfh)
    {
        LogTrace(Channel::Web, "Adding static file handler");
        m_SFH = std::move(sfh);
    }

    HTTP::Message Router::HTTP_OnRequest(HTTP::Request const& req)
    {
        try
        {
            std::filesystem::path static_file_result;
            HTTP::Routing::matches m;

            std::string_view* matches_it = m.matches();
            std::string_view* ids_it = m.ids();

            if (auto path = boost::urls::parse_path(req.target()); path.has_error())
            {
                LogDebug(Channel::Web, std::format("Supplied path could not be parsed; error was -> {}", path.error().message()));
                return HTTP::Responses::Response_400(req);
            }
            else if (auto p = m_HttpRoutes.find_impl(*path, matches_it, ids_it); nullptr != p)
            {
                LogTrace(Channel::Web, std::format("Handling HTTP {} request for {}", magic_enum::enum_name(req.method()), req.target()));
                m.resize(static_cast<std::size_t>(matches_it - m.matches()));
                return p->OnRequest(req);
            }
            else if (m_SFH && m_SFH->match(req.target(), static_file_result))
            {
                LogTrace(Channel::Web, std::format("Attempting to serve static content; file is -> {}", static_file_result.string()));
                return HTTP::Responses::Response_StaticFile(req, static_file_result);
            }
            else
            {
                LogDebug(Channel::Web, std::format("Path '{}' was requested but no handler was available", req.target()));
            }
        }
        catch (const std::exception& ex)
        {
            LogDebug(Channel::Web, std::format("An exception was thrown while processing an HTTP request: exception was -> {}", ex.what()));
            return HTTP::Responses::Response_500(req);
        }

        LogDebug(Channel::Web, "Could not handle request -> returning a 404 NOT FOUND");
        return HTTP::Responses::Response_404(req);
    }

}
// namespace AqualinkAutomate::HTTP