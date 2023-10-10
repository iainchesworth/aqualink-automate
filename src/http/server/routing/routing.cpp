#include <filesystem>
#include <format>
#include <optional>

#include <boost/url/parse.hpp>
#include <boost/url/parse_path.hpp>
#include <boost/url/url.hpp>
#include <magic_enum.hpp>

#include "exceptions/exception_http_duplicateroute.h"
#include "formatters/beast_stringview_formatter.h"
#include "formatters/url_segments_encoded_view_formatter.h"
#include "http/server/static_file_handler.h"
#include "http/server/responses/response_staticfile.h"
#include "http/server/responses/response_400.h"
#include "http/server/responses/response_404.h"
#include "http/server/responses/response_500.h"
#include "http/server/routing/matches.h"
#include "http/server/routing/node.h"
#include "http/server/routing/routing.h"
#include "interfaces/isession.h"
#include "interfaces/iwebroute.h"
#include "interfaces/iwebsocket.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP::Routing
{

	namespace
	{

		std::vector<std::unique_ptr<Interfaces::IWebRouteBase>> http_routes_vec{};
		std::vector<std::unique_ptr<Interfaces::IWebSocketBase>> ws_routes_vec{};

		HTTP::Routing::impl<Interfaces::IWebRouteBase> http_routes{};
		HTTP::Routing::impl<Interfaces::IWebSocketBase> ws_routes{};

		std::optional<StaticFileHandler> sf_route;

	}
	// unnamed namespace

	void Add(std::unique_ptr<Interfaces::IWebRouteBase>&& handler)
	{
		LogTrace(Channel::Web, std::format("Adding HTTP handler for route '{}'", handler->Route()));

		auto& handler_ref = http_routes_vec.emplace_back(std::move(handler));

		http_routes.insert_impl(handler_ref->Route(), handler_ref.get());
	}

	void Add(std::unique_ptr<Interfaces::IWebSocketBase>&& handler)
	{
		LogTrace(Channel::Web, std::format("Adding WebSocket handler for route '{}'", handler->Route()));

		auto& handler_ref = ws_routes_vec.emplace_back(std::move(handler));

		ws_routes.insert_impl(handler_ref->Route(), handler_ref.get());
	}

	void StaticHandler(StaticFileHandler&& sf)
	{
		LogTrace(Channel::Web, "Adding static file handler");
		sf_route = std::move(sf);
	}

	HTTP::Message HTTP_OnRequest(const HTTP::Request& req)
	{
		try
		{
			std::filesystem::path static_file_result;
			HTTP::Routing::matches m;

			std::string_view* matches_it = m.matches();
			std::string_view* ids_it = m.ids();

			if (auto path = boost::urls::parse_path(req.target()); path.has_error())
			{
				LogDebug(Channel::Web, std::format("Supplied http path could not be parsed; error was -> {}", path.error().message()));
				return HTTP::Responses::Response_400(req);
			}
			else if (auto p = http_routes.find_impl(*path, matches_it, ids_it); nullptr != p)
			{
				LogTrace(Channel::Web, std::format("Handling HTTP {} request for {}", magic_enum::enum_name(req.method()), req.target()));
				m.resize(static_cast<std::size_t>(matches_it - m.matches()));
				return p->OnRequest(req);
			}
			else if (sf_route.has_value() && sf_route->match(req.target(), static_file_result))
			{
				LogTrace(Channel::Web, std::format("Attempting to serve static content; file is -> {}", static_file_result.string()));
				return HTTP::Responses::Response_StaticFile(req, static_file_result);
			}
			else
			{
				LogDebug(Channel::Web, std::format("Path '{}' was requested but no HTTP handler was available", req.target()));
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

	Interfaces::IWebSocketBase* WS_OnAccept(const std::string_view target)
	{
		try
		{
			HTTP::Routing::matches m;

			std::string_view* matches_it = m.matches();
			std::string_view* ids_it = m.ids();

			if (auto path = boost::urls::parse_path(target); path.has_error())
			{
				LogDebug(Channel::Web, std::format("Supplied websocket path could not be parsed; error was -> {}", path.error().message()));
			}
			else if (auto p = ws_routes.find_impl(*path, matches_it, ids_it); nullptr == p)
			{
				LogDebug(Channel::Web, std::format("Path '{}' was requested but no WS handler was available", target));
			}
			else
			{
				LogTrace(Channel::Web, std::format("Handling WS request for {}", target));
				m.resize(static_cast<std::size_t>(matches_it - m.matches()));
				return p;
			}
		}
		catch (const std::exception& ex)
		{
			LogDebug(Channel::Web, std::format("An exception was thrown while processing an WS request: exception was -> {}", ex.what()));
		}

		LogDebug(Channel::Web, "Could not handle WS request -> returning nullptr");
		return nullptr;
	}

}
// namespace AqualinkAutomate::HTTP::Routing
