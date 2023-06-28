#pragma once

#include <crow/app.h>

#include "http/webroute_types.h"
#include "interfaces/iwebroute.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char VERSION_ROUTE_URL[] = "/api/version";

	class WebRoute_Version : public Interfaces::IWebRoute<VERSION_ROUTE_URL>
	{
	public:
		WebRoute_Version(crow::SimpleApp& app);

	public:
		void WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp);
	};

}
// namespace AqualinkAutomate::HTTP
