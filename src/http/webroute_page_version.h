#pragma once

#include "http/webroute_types.h"
#include "interfaces/iwebpageroute.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char PAGE_VERSION_ROUTE_URL[] = "/version";
	constexpr const char PAGE_VERSION_TEMPLATE[] = "templates/version.html.mustache";

	class WebRoute_Page_Version : public Interfaces::IWebPageRoute<PAGE_VERSION_ROUTE_URL, PAGE_VERSION_TEMPLATE>
	{
	public:
		WebRoute_Page_Version(HTTP::Server& http_server);

	public:
		virtual void WebRequestHandler(HTTP::Request& req, HTTP::Response& resp) override;
	};

}
// namespace AqualinkAutomate::HTTP
