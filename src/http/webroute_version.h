#pragma once

#include "http/webroute_types.h"
#include "interfaces/iwebroute.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char VERSION_ROUTE_URL[] = "/api/version";

	class WebRoute_Version : public Interfaces::IWebRoute<VERSION_ROUTE_URL>
	{
	public:
		WebRoute_Version(HTTP::Server& http_server);

	public:
		void WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp);
	};

}
// namespace AqualinkAutomate::HTTP
