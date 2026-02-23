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
        virtual HTTP::Message OnRequest(const HTTP::Request& req) = 0;
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
