#pragma once

#include <crow/app.h>

#include "http/webroute_types.h"
#include "interfaces/iwebroute.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char PAGE_VERSION_ROUTE_URL[] = "/version";

	class WebRoute_Page_Version : public Interfaces::IWebRoute<PAGE_VERSION_ROUTE_URL>
	{
	public:
		WebRoute_Page_Version(crow::SimpleApp& app);

	public:
		void WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp);
	};

}
// namespace AqualinkAutomate::HTTP
