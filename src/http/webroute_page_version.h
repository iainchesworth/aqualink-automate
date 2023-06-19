#pragma once

#include <crow/app.h>

#include "interfaces/iwebroute.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char PAGE_VERSION_ROUTE_URL[] = "/version";

	class WebRoute_Page_Version : public Interfaces::IWebRoute<PAGE_VERSION_ROUTE_URL>
	{
	public:
		WebRoute_Page_Version(crow::SimpleApp& app);

	public:
		void WebRequestHandler(const Request& req, Response& resp);
	};

}
// namespace AqualinkAutomate::HTTP
