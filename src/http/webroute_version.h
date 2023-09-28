#pragma once

#include "interfaces/iwebroute.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char VERSION_ROUTE_URL[] = "/api/version";

	class WebRoute_Version : public Interfaces::IWebRoute<VERSION_ROUTE_URL>
	{
    public:
        WebRoute_Version() = default;
        virtual ~WebRoute_Version() = default;

	public:
        virtual HTTP::Message OnRequest(HTTP::Request req) final;
	};

}
// namespace AqualinkAutomate::HTTP
