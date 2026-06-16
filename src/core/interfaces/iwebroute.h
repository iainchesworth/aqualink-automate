#pragma once

#include <string_view>

#include "concepts/is_c_array.h"
#include "http/server/server_types.h"

namespace AqualinkAutomate::Interfaces
{	
	class IWebRouteBase
    {
    public:
        IWebRouteBase() = default;
        virtual ~IWebRouteBase() = default;

    public:
        virtual const std::string_view Route() const = 0;

    public:
        // Returns a mutable Response (not a type-erased message_generator) so the
        // router can stamp response-wide policy — e.g. Cache-Control: no-store on
        // dynamic API data — in one place before serialising. See HTTP_OnRequest.
        virtual HTTP::Response OnRequest(const HTTP::Request& req) = 0;

        // Per-route opt-out of the control-plane security policy. Registered routes
        // are gated by SecurityConfig (bearer token / Origin allow-list / CSRF)
        // BEFORE dispatch; a route that returns false here is dispatched WITHOUT
        // those checks. Used by the unauthenticated liveness probe (/api/health) so
        // a container/orchestrator health check can reach it without baking in the
        // operator's secret token. Defaults to true — every other route stays gated.
        virtual bool RequiresAuthentication() const { return true; }
    };

	template<const auto& ROUTE_URL>
	requires (Concepts::CArray<decltype(ROUTE_URL)>)
    class IWebRoute : public IWebRouteBase
	{
    public:
        IWebRoute() = default;
        virtual ~IWebRoute() = default;

	public:
        virtual const std::string_view Route() const final
		{
            return ROUTE_URL;
		}
	};

}
// namespace AqualinkAutomate::Interfaces
