#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <string_view>

#include <boost/beast/core/string.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/parse_path.hpp>
#include <boost/url/url.hpp>
#include <magic_enum/magic_enum.hpp>

#include "exceptions/exception_http_duplicateroute.h"
#include "formatters/beast_stringview_formatter.h"
#include "formatters/url_segments_encoded_view_formatter.h"
#include "http/server/server_fields.h"
#include "http/server/static_file_handler.h"
#include "http/server/responses/response_staticfile.h"
#include "http/server/responses/response_400.h"
#include "http/server/responses/response_404.h"
#include "http/server/responses/response_500.h"
#include "http/server/routing/matches.h"
#include "http/server/routing/node.h"
#include "http/server/routing/routing.h"
#include "interfaces/iwebroute.h"
#include "interfaces/iwebsocket.h"
#include "logging/logging.h"
#include "profiling/factories/profiler_factory.h"
#include "profiling/factories/profiling_unit_factory.h"

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

		SecurityConfig security_config{};

		// Attaching descriptive text to a profiling zone is a no-op in every
		// non-Tracy build (the base IProfilingUnit::Text() does nothing), so building
		// the std::format string unconditionally is pure waste on the request hot
		// path. Compile the formatting out unless a profiler backend is enabled; the
		// callable is only invoked when the text will actually be recorded.
		template<typename FN>
		void DeferredZoneText([[maybe_unused]] const Types::ProfilingUnitTypePtr& zone, [[maybe_unused]] FN&& make_text)
		{
#if defined(TRACY_ENABLE) || defined(VTUNE_SUPPORT_ENABLED) || defined(UProf_SUPPORT_ENABLED)
			if (zone)
			{
				zone->Text(std::forward<FN>(make_text)());
			}
#endif
		}

		// Constant-time comparison of two byte sequences. Always inspects the full
		// length of both operands so the running time does not leak how many leading
		// characters matched (mitigating a timing side-channel on the shared token).
		[[nodiscard]] bool ConstantTimeEquals(std::string_view lhs, std::string_view rhs) noexcept
		{
			// Seed the accumulator with a definite non-zero value when the lengths
			// differ (avoiding a low-bits-only fold that could collide), then OR in the
			// per-byte differences over the longer span so the loop never short-circuits.
			unsigned int diff = (lhs.size() == rhs.size()) ? 0U : 1U;

			const std::size_t n = std::max(lhs.size(), rhs.size());
			for (std::size_t i = 0; i < n; ++i)
			{
				const unsigned int a = (i < lhs.size()) ? static_cast<unsigned char>(lhs[i]) : 0U;
				const unsigned int b = (i < rhs.size()) ? static_cast<unsigned char>(rhs[i]) : 0U;
				diff |= (a ^ b);
			}

			return diff == 0U;
		}

		// Build a small text/plain error response (401/403) mirroring the existing
		// Response_4xx helpers. The body is intentionally generic so we never echo
		// the supplied (or expected) token back to the caller.
		[[nodiscard]] HTTP::Response MakeSecurityResponse(const HTTP::Request& req, HTTP::Status status, std::string_view body)
		{
			boost::beast::http::response<boost::beast::http::string_body> res{ status, req.version() };
			res.set(boost::beast::http::field::server, ServerFields::Server());
			res.set(boost::beast::http::field::content_type, ContentTypes::TEXT_PLAIN);
			if (status == HTTP::Status::unauthorized)
			{
				// RFC 7235: a 401 SHOULD carry a WWW-Authenticate challenge.
				res.set(boost::beast::http::field::www_authenticate, "Bearer");
			}
			res.keep_alive(req.keep_alive());
			res.body() = std::string(body);
			res.prepare_payload();
			return res;
		}

		// True for HTTP methods that mutate server state and therefore warrant the
		// optional CSRF custom-header requirement.
		[[nodiscard]] bool IsStateChangingMethod(boost::beast::http::verb method) noexcept
		{
			switch (method)
			{
			case boost::beast::http::verb::post:
			case boost::beast::http::verb::put:
			case boost::beast::http::verb::patch:
			case boost::beast::http::verb::delete_:
				return true;
			default:
				return false;
			}
		}

		// Beast's field value type (boost::beast::string_view) is not always the same
		// type as std::string_view; convert via data()/size() so this compiles on any
		// Boost version.
		template<typename SV>
		[[nodiscard]] std::string_view ToStringView(const SV& sv) noexcept
		{
			return std::string_view{ sv.data(), sv.size() };
		}

		// Look up a header value as a std::string_view, returning an empty view when
		// the header is absent.
		template<typename FIELD>
		[[nodiscard]] std::string_view HeaderValue(const HTTP::Request& req, FIELD field)
		{
			const auto it = req.find(field);
			return (it != req.end()) ? ToStringView(it->value()) : std::string_view{};
		}

		// Extract and constant-time-compare a bearer token offered via the
		// WebSocket handshake's Sec-WebSocket-Protocol header.  Browsers cannot set
		// an Authorization header on a WebSocket upgrade, so the UI offers the token
		// as a `bearer.<token>` subprotocol entry alongside the `aqualink` marker
		// (e.g. "aqualink, bearer.<token>").  NEVER logs the token.
		[[nodiscard]] bool WebSocketSubprotocolTokenMatches(const HTTP::Request& req, std::string_view expected_token)
		{
			const std::string_view header = HeaderValue(req, boost::beast::http::field::sec_websocket_protocol);
			static constexpr std::string_view BEARER_ENTRY_PREFIX{ "bearer." };

			std::size_t pos = 0;
			while (pos <= header.size())
			{
				const std::size_t comma = header.find(',', pos);
				std::string_view entry = (comma == std::string_view::npos)
					? header.substr(pos)
					: header.substr(pos, comma - pos);

				// Trim surrounding optional whitespace (per the header's ABNF).
				while (!entry.empty() && (entry.front() == ' ' || entry.front() == '\t')) { entry.remove_prefix(1); }
				while (!entry.empty() && (entry.back() == ' ' || entry.back() == '\t')) { entry.remove_suffix(1); }

				if (entry.starts_with(BEARER_ENTRY_PREFIX))
				{
					if (ConstantTimeEquals(entry.substr(BEARER_ENTRY_PREFIX.size()), expected_token))
					{
						return true;
					}
				}

				if (comma == std::string_view::npos) { break; }
				pos = comma + 1;
			}

			return false;
		}

		// Evaluate the active SecurityConfig against a parsed request. Returns the
		// rejection response to send when the request is denied, or std::nullopt when
		// it is permitted. Shared by the HTTP and WebSocket-upgrade paths so both
		// enforce identical rules. NEVER logs the configured or supplied token.
		[[nodiscard]] std::optional<HTTP::Response> EvaluateSecurity(const HTTP::Request& req, bool is_websocket_upgrade)
		{
			const SecurityConfig& cfg = security_config;
			if (!cfg.IsEnabled())
			{
				// Feature fully disabled (the default) -> identical to historical behaviour.
				return std::nullopt;
			}

			// --- Origin allow-list (primary cross-site / cross-origin defence) ---
			if (!cfg.AllowedOrigins.empty())
			{
				const std::string_view origin = HeaderValue(req, boost::beast::http::field::origin);

				const bool origin_allowed = !origin.empty() &&
					std::any_of(cfg.AllowedOrigins.begin(), cfg.AllowedOrigins.end(),
						[origin](const std::string& allowed) { return allowed == origin; });

				if (!origin_allowed)
				{
					LogWarning(Channel::Web, [&] { return std::format("Rejected {} request: Origin '{}' is not in the allow-list", is_websocket_upgrade ? "WebSocket upgrade" : "HTTP", origin); });
					return MakeSecurityResponse(req, HTTP::Status::forbidden, "Forbidden: origin not allowed.");
				}
			}

			// --- Bearer token authentication ---
			if (cfg.AuthToken.has_value())
			{
				static constexpr std::string_view BEARER_PREFIX{ "Bearer " };

				const std::string_view header = HeaderValue(req, boost::beast::http::field::authorization);

				bool authorised = false;
				if (header.starts_with(BEARER_PREFIX))
				{
					const std::string_view presented = header.substr(BEARER_PREFIX.size());
					authorised = ConstantTimeEquals(presented, *cfg.AuthToken);
				}

				// Browsers cannot attach an Authorization header to a WebSocket
				// upgrade, so for upgrades also accept the token carried in the
				// Sec-WebSocket-Protocol header as a `bearer.<token>` entry.
				if (!authorised && is_websocket_upgrade)
				{
					authorised = WebSocketSubprotocolTokenMatches(req, *cfg.AuthToken);
				}

				if (!authorised)
				{
					LogWarning(Channel::Web, [&] { return std::format("Rejected unauthenticated {} request (missing/invalid bearer token)", is_websocket_upgrade ? "WebSocket upgrade" : "HTTP"); });
					return MakeSecurityResponse(req, HTTP::Status::unauthorized, "Unauthorized.");
				}
			}

			// --- CSRF mitigation for state-changing requests ---
			// (Not applicable to the WebSocket upgrade, which is always a GET.)
			if (cfg.RequireCsrfHeader && !is_websocket_upgrade && IsStateChangingMethod(req.method()))
			{
				const std::string_view csrf = HeaderValue(req, boost::beast::string_view{ "X-Requested-With" });
				if (csrf.empty())
				{
					LogWarning(Channel::Web, [&] { return std::format("Rejected state-changing {} request: missing X-Requested-With header", magic_enum::enum_name(req.method())); });
					return MakeSecurityResponse(req, HTTP::Status::forbidden, "Forbidden: missing CSRF header.");
				}
			}

			return std::nullopt;
		}

	}
	// unnamed namespace

	void Clear()
	{
		http_routes_vec.clear();
		ws_routes_vec.clear();
		http_routes = HTTP::Routing::impl<Interfaces::IWebRouteBase>{};
		ws_routes = HTTP::Routing::impl<Interfaces::IWebSocketBase>{};
		sf_route.reset();
		security_config = SecurityConfig{};
	}

	void SetSecurityConfig(SecurityConfig config)
	{
		security_config = std::move(config);

		if (security_config.IsEnabled())
		{
			LogInfo(Channel::Web, [&]
				{
					return std::format("HTTP/WS security policy enabled (auth={}, origin_allowlist={}, csrf_header={})",
						security_config.AuthToken.has_value(),
						security_config.AllowedOrigins.size(),
						security_config.RequireCsrfHeader);
				});
		}
	}

	const SecurityConfig& GetSecurityConfig()
	{
		return security_config;
	}

	void Add(std::unique_ptr<Interfaces::IWebRouteBase>&& handler)
	{
		auto& handler_ref = http_routes_vec.emplace_back(std::move(handler));

		LogTrace(Channel::Web, [&] { return std::format("Adding HTTP handler for route '{}'", handler_ref->Route()); });

		http_routes.insert_impl(handler_ref->Route(), handler_ref.get());
	}

	void Add(std::unique_ptr<Interfaces::IWebSocketBase>&& handler)
	{
		auto& handler_ref = ws_routes_vec.emplace_back(std::move(handler));

		LogTrace(Channel::Web, [&] { return std::format("Adding WebSocket handler for route '{}'", handler_ref->Route()); });

		ws_routes.insert_impl(handler_ref->Route(), handler_ref.get());
	}

	void StaticHandler(StaticFileHandler&& sf)
	{
		LogTrace(Channel::Web, "Adding static file handler");
		sf_route = std::move(sf);
	}

	HTTP::Message HTTP_OnRequest(const HTTP::Request& req)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Routing::RouteRequest", std::source_location::current());
		DeferredZoneText(zone, [&] { return std::format("{} {}", magic_enum::enum_name(req.method()), std::string_view(req.target())); });

		try
		{
			// Security is enforced PER-BRANCH below rather than up front: registered
			// routes (incl. /api and /metrics) and unmatched paths are gated, but
			// static assets are served WITHOUT authentication so a token-protected
			// deployment can still load index.html / scripts / css to render the
			// login screen.  When the policy is disabled (the default) EvaluateSecurity
			// is a cheap no-op, so behaviour is byte-identical to before.
			std::filesystem::path static_file_result;
			HTTP::Routing::matches m;

			std::string_view* matches_it = m.matches();
			std::string_view* ids_it = m.ids();
			std::string_view* matches_end = m.matches() + m.size();
			std::string_view* ids_end = m.ids() + m.size();

			if (auto path = boost::urls::parse_path(req.target()); path.has_error())
			{
				Factory::ProfilerFactory::Instance().Get()->Message("HTTP 400 Bad Request");
				LogDebug(Channel::Web, [&] { return std::format("Supplied http path could not be parsed; error was -> {}", path.error().message()); });
				return HTTP::Responses::Response_400(req);
			}
			else if (auto p = http_routes.find_impl(*path, matches_it, ids_it, matches_end, ids_end); nullptr != p)
			{
				// Registered route -> enforce security before dispatch.
				if (auto rejection = EvaluateSecurity(req, false); rejection.has_value())
				{
					return std::move(*rejection);
				}

				LogTrace(Channel::Web, [&] { return std::format("Handling HTTP {} request for {}", magic_enum::enum_name(req.method()), std::string_view(req.target())); });
				return p->OnRequest(req);
			}
			else if (sf_route.has_value() && sf_route->match(req.target(), static_file_result))
			{
				// Static asset -> intentionally UNauthenticated (see above).
				LogTrace(Channel::Web, [&] { return std::format("Attempting to serve static content; file is -> {}", static_file_result.string()); });
				return HTTP::Responses::Response_StaticFile(req, static_file_result);
			}
			else
			{
				// Unmatched path -> still enforce security so an unknown /api/* path
				// answers 401 (not 404) when a token is required.
				if (auto rejection = EvaluateSecurity(req, false); rejection.has_value())
				{
					return std::move(*rejection);
				}

				Factory::ProfilerFactory::Instance().Get()->Message("HTTP 404 Not Found");
				LogDebug(Channel::Web, [&] { return std::format("Path '{}' was requested but no HTTP handler was available", std::string_view(req.target())); });
				LogDebug(Channel::Web, "Could not handle request -> returning a 404 NOT FOUND");
				return HTTP::Responses::Response_404(req);
			}
		}
		catch (const std::exception& ex)
		{
			// The detail stays server-side only; the client receives a context-free 500.
			// Promote to Warning so an escaping exception is visible at the default log
			// level (it indicates a route handler fault, not routine traffic).
			LogWarning(Channel::Web, [&] { return std::format("An exception was thrown while processing an HTTP request: exception was -> {}", ex.what()); });
			return HTTP::Responses::Response_500(req);
		}
	}

	Interfaces::IWebSocketBase* WS_OnAccept(const std::string_view target)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Routing::RouteWebSocket", std::source_location::current());
		DeferredZoneText(zone, [&] { return std::string(target); });

		try
		{
			HTTP::Routing::matches m;

			std::string_view* matches_it = m.matches();
			std::string_view* ids_it = m.ids();
			std::string_view* matches_end = m.matches() + m.size();
			std::string_view* ids_end = m.ids() + m.size();

			if (auto path = boost::urls::parse_path(target); path.has_error())
			{
				LogDebug(Channel::Web, [&] { return std::format("Supplied websocket path could not be parsed; error was -> {}", path.error().message()); });
			}
			else if (auto p = ws_routes.find_impl(*path, matches_it, ids_it, matches_end, ids_end); nullptr == p)
			{
				LogDebug(Channel::Web, [&] { return std::format("Path '{}' was requested but no WS handler was available", target); });
			}
			else
			{
				LogTrace(Channel::Web, [&] { return std::format("Handling WS request for {}", target); });
				return p;
			}
		}
		catch (const std::exception& ex)
		{
			LogWarning(Channel::Web, [&] { return std::format("An exception was thrown while processing a WS request: exception was -> {}", ex.what()); });
		}

		LogDebug(Channel::Web, "Could not handle WS request -> returning nullptr");
		return nullptr;
	}

	std::optional<HTTP::Response> AuthorizeWebSocketUpgrade(const HTTP::Request& req)
	{
		return EvaluateSecurity(req, true);
	}

}
// namespace AqualinkAutomate::HTTP::Routing
