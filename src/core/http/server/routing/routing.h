#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "http/server/server_types.h"
#include "http/server/static_file_handler.h"
#include "interfaces/iwebroute.h"
#include "interfaces/iwebsocket.h"

namespace AqualinkAutomate::HTTP::Routing
{

	/// Optional, opt-in security policy for the HTTP/WebSocket control plane.
	///
	/// Every knob defaults to "disabled" so a default-constructed SecurityConfig
	/// reproduces the historical behaviour exactly: no authentication, no Origin
	/// check, no CSRF requirement.  The policy is consulted by HTTP_OnRequest (for
	/// HTTP requests) and AuthorizeWebSocketUpgrade (for the WebSocket handshake).
	struct SecurityConfig
	{
		/// Shared bearer token. When unset (the default) no authentication is
		/// performed. When set, requests must carry "Authorization: Bearer <token>"
		/// and the token is compared in constant time.
		std::optional<std::string> AuthToken{ std::nullopt };

		/// Allow-list of acceptable Origin header values for cross-site protection.
		/// When empty (the default) the Origin header is not checked. When non-empty,
		/// a request/upgrade carrying an Origin not in the list is rejected.
		std::vector<std::string> AllowedOrigins{};

		/// When true, state-changing requests (POST/PUT/PATCH/DELETE) must carry a
		/// non-empty custom header (X-Requested-With) as a CSRF mitigation. Defaults
		/// to false so existing clients are unaffected.
		bool RequireCsrfHeader{ false };

		/// True when any security knob is engaged. Used to early-out of the checks on
		/// the request hot path when the feature is entirely disabled (the default).
		[[nodiscard]] bool IsEnabled() const noexcept
		{
			return AuthToken.has_value() || !AllowedOrigins.empty() || RequireCsrfHeader;
		}
	};

	void Clear();

	void Add(std::unique_ptr<Interfaces::IWebRouteBase>&& handler);
	void Add(std::unique_ptr<Interfaces::IWebSocketBase>&& handler);

	void StaticHandler(StaticFileHandler&& sf);

	/// Install (or replace) the active security policy. Reset to the disabled
	/// default by Clear(). The HttpServer wires this from the Web settings.
	void SetSecurityConfig(SecurityConfig config);
	const SecurityConfig& GetSecurityConfig();

	/// peer_ip (the connecting client's address, empty when unknown) feeds the
	/// per-source failed-auth rate limiter; pass it from the HTTP session.
	HTTP::Message HTTP_OnRequest(const HTTP::Request& req, std::string_view peer_ip = {});
	Interfaces::IWebSocketBase* WS_OnAccept(const std::string_view target);

	/// Evaluate the security policy against a WebSocket upgrade request.
	/// Returns std::nullopt when the upgrade is permitted; otherwise returns the
	/// HTTP error response (401/403/429) that should be written instead of accepting.
	std::optional<HTTP::Response> AuthorizeWebSocketUpgrade(const HTTP::Request& req, std::string_view peer_ip = {});

}
// namespace AqualinkAutomate::HTTP::Routing
